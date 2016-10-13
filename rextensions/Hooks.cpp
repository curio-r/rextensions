#include "stdafx.h"
#include "Hooks.h"
#include "enum.h"
#include "MapColorTable.h"
#include "astar.h"

#include <chrono>
#include <algorithm>

#define MSG_COLOR_WARNING 0x00FFFFu
#define MSG_COLOR_ERROR 0x0000FFu
#define MSG_COLOR_INFO 0xFFFF00u

static const auto g_timeBegin = std::chrono::steady_clock::now();
static bool g_mustUpdateMapColor = false;

#define LOG_VERBOSE 0

size_t timeGetTime_(void)
{
	return (size_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - g_timeBegin).count();
}

static char msg[256];

static size_t g_mapWidth;
static size_t g_mapHeight;
static std::vector<CAttrCell> g_mapCells;
static std::string g_mapName;
static bool g_isIdleMode = false;
static bool g_printPickInfo = false;
static size_t g_currentTarget = 0;
static std::vector<CActorPickInfo> g_pickInfo;
static int g_playerX = 0;
static int g_playerY = 0;
static int g_playerDestX = 0;
static int g_playerDestY = 0;
static int g_playerMoveStart = 0;
static int g_mouseX = 0;
static int g_mouseY = 0;
static bool g_mouseClick = false;
static bool g_mustUpdatePlayerPos = true;

// Pathfinding

struct RagGrid
{
	typedef tuple<int, int> Location;
	static array<Location, 8> DIRS;

	inline bool in_bounds(Location id) const 
	{
		int x, y;
		tie(x, y) = id;
		return 0 <= x && x < (int)g_mapWidth && 0 <= y && y < (int)g_mapHeight;
	}

	inline bool passable(Location id) const 
	{
		int x, y;
	
		tie(x, y) = id;

		const auto &cell = g_mapCells[y * g_mapWidth + x];

		return cell.flag != 1 && cell.flag != 5;
	}

	inline float cost(const Location &a, const Location &b) const
	{
		int x1, y1, x2, y2;

		tie(x1, y1) = a;
		tie(x2, y2) = b;

		float dx = (float)abs(x1 - x2);
		float dy = (float)abs(y1 - y2);

		return sqrtf(dx * dx + dy*dy);
	}

	vector<Location> neighbors(Location id) const {
		int x, y, dx, dy;
		tie(x, y) = id;
		vector<Location> results;

		for (auto dir : DIRS) {
			tie(dx, dy) = dir;
			Location next(x + dx, y + dy);
			if (in_bounds(next) && passable(next)) {
				results.push_back(next);
			}
		}

		if ((x + y) % 2 == 0) {
			// aesthetic improvement on square grids
			std::reverse(results.begin(), results.end());
		}

		return results;
	}
} g_grid;

inline float RagHeuristic(RagGrid::Location a, RagGrid::Location b) {

	int x1, y1, x2, y2;

	tie(x1, y1) = a;
	tie(x2, y2) = b;

	float dx = (float)abs(x1 - x2);
	float dy = (float)abs(y1 - y2);

	return sqrtf(dx * dx + dy*dy);
}

array<RagGrid::Location, 8> RagGrid::DIRS
{
	Location{ 1, 0 },
	Location{ 0, -1 },
	Location{ -1, 0 },
	Location{ 0, 1 },
	Location{ -1, -1 },
	Location{ 1, 1 },
	Location{ -1, 1 },
	Location{ 1, -1 }
};

template<typename Graph>
bool a_star_search
(const Graph& graph,
typename Graph::Location start,
typename Graph::Location goal,
unordered_map<typename Graph::Location, typename Graph::Location>& came_from,
unordered_map<typename Graph::Location, float>& cost_so_far)
{
	typedef typename Graph::Location Location;
	PriorityQueue<Location, float> frontier;
	frontier.put(start, 0);

	came_from[start] = start;
	cost_so_far[start] = 0;

	auto i = 0;

	while (!frontier.empty()) {

		i++;

		if (i > 1500)
		{
			// Give up
			return false;
		}

		auto current = frontier.get();

		if (current == goal) {
			return true;
		}

		for (auto next : graph.neighbors(current)) {
			float new_cost = cost_so_far[current] + graph.cost(current, next);
			if (!cost_so_far.count(next) || new_cost < cost_so_far[next]) {
				cost_so_far[next] = new_cost;
				float priority = new_cost + RagHeuristic(next, goal);
				frontier.put(next, priority);
				came_from[next] = current;
			}
		}
	}

	return false;
}

std::unordered_map<RagGrid::Location, RagGrid::Location> g_pathCameFrom;
std::unordered_map<RagGrid::Location, float> g_pathCostSoFar;
std::vector<RagGrid::Location> g_path;
size_t g_pathOffset = 0;
int g_goalX;
int g_goalY;
bool g_mustRecalculatePath = true;
bool g_waitingForMove = true;

static bool FindPath(int sx, int sy, int dx, int dy)
{
	RagGrid::Location start { sx, sy };
	RagGrid::Location goal { dx, dy };

	g_pathCameFrom.clear();
	g_pathCostSoFar.clear();

	auto result = a_star_search<RagGrid>(g_grid, start, goal, g_pathCameFrom, g_pathCostSoFar);

	if (!result)
		return false;

	g_path = reconstruct_path(start, goal, g_pathCameFrom);

	g_mustRecalculatePath = false;
	g_pathOffset = 0;

	return true;
}

// Idle mode

void PickGoal()
{
	int x = 0;
	int y = 0;

	bool valid_goal = false;

	do
	{
		do
		{
			x = rand() % g_mapWidth;
			y = rand() % g_mapHeight;

		} while (std::abs(x - g_playerDestX) > 150 ||
			std::abs(y - g_playerDestY) > 150 ||
			(g_mapCells[x + y * g_mapWidth].flag & 1) == 1);

		valid_goal = FindPath(g_playerDestX, g_playerDestY, x, y);

	} while (!valid_goal);

	g_goalX = x;
	g_goalY = y;

#if LOG_VERBOSE
	sprintf_s(msg, "New goal is (%d, %d)", g_goalX, g_goalY);

	proxy(UIWindowMgr)::SendMsg(g_windowMgr, UIM_PUSHINTOCHATHISTORY, (int)msg, MSG_COLOR_INFO, 0, 0);
#endif
}

void EncodePosDir(int srcx, int srcy, int srcdir, unsigned char *result)
{
	result[0] = srcx >> 2;
	result[1] = ((BYTE)srcx << 6) ^ (((BYTE)srcx << 6) ^ (srcy >> 4)) & 0x3F;
	result[2] = 16 * srcy ^ (srcdir ^ 16 * srcy) & 0xF;
}

static void DecodeSrcDst(unsigned char *packet, int &rsx, int &rsy, int &rdx, int &rdy, int &xcellpos, int &ycellpos)
{
	rsx = ((packet[0] & 0xFF) << 2) | ((packet[1] & 0xC0) >> 6);
	rsy = ((packet[1] & 0x3F) << 4) | ((packet[2] & 0xF0) >> 4);
	rdx = ((packet[3] & 0xFC) >> 2) | ((packet[2] & 0x0F) << 6);
	rdy = ((packet[3] & 0x03) << 8) | ((packet[4] & 0xFF));
	xcellpos = 0;
	ycellpos = 0;
}

static void RequestMove(int x, int y)
{
#pragma pack(push, 1)
	struct PACKET_CZ_REQUEST_MOVE {
		short PacketType;
		unsigned char dest[3];
	};
#pragma pack(pop)

	PACKET_CZ_REQUEST_MOVE p;

	p.PacketType = 0x0437; // 2012-04-10
	
	EncodePosDir(x, y, 0, p.dest);

	proxy(CRagConnection)::SendPacket(proxy(CRagConnection)::instanceR(), sizeof(p), (char *)&p);

	g_waitingForMove = true;
}

static size_t g_lastReset = 0;

static void FollowPlan()
{
	if (g_playerX == 0 || g_playerY == 0)
	{
		return;
	}

	if ((g_goalX == 0 && g_goalY == 0) || g_mustRecalculatePath ||
		(g_waitingForMove && (int)timeGetTime_() > g_playerMoveStart + 5000 &&
		timeGetTime_() > g_lastReset + 5000))
	{
		PickGoal();

		g_lastReset = timeGetTime_();
		g_waitingForMove = false;
	}

	//if (g_mustRecalculatePath)
	//{
	//	FindPath(g_playerDestX, g_playerDestY, g_goalX, g_goalY);
	//}

	auto a = ((timeGetTime_() % 3600) / 3600.0f) * 2 * 3.1415f;

	g_mouseX = (int)(g_renderer->m_width / 2 + 100 * std::cosf(a));
	g_mouseY = (int)(g_renderer->m_height / 2 + 100 * std::sinf(a));

	auto distanceToPlayer = [](int x, int y)
	{
		return std::sqrt(std::pow(std::abs(x - g_playerX), 2) + std::pow(std::abs(y - g_playerY), 2));
	};

	if ((int)timeGetTime_() > g_playerMoveStart + 500 && !g_waitingForMove)
	{
#if LOG_VERBOSE
		sprintf_s(msg, "Proceeding with plan...");

		proxy(UIWindowMgr)::SendMsg(g_windowMgr, UIM_PUSHINTOCHATHISTORY, (int)msg, MSG_COLOR_INFO, 0, 0);
#endif

		if (std::abs(g_playerDestX - g_goalX) < 5 &&
			std::abs(g_playerDestY - g_goalY) < 5)
		{
			PickGoal();
		}

		for (auto i = g_pathOffset; i < g_path.size(); i++)
		{
			int x, y;

			std::tie(x, y) = g_path[i];

			auto dist = distanceToPlayer(x, y);

			if (dist > 10 || (i + 1) == g_path.size())
			{
#if LOG_VERBOSE
				sprintf_s(msg, "Requesting move from (%d %d) to (%d, %d)", g_playerDestX, g_playerDestY, x, y);

				proxy(UIWindowMgr)::SendMsg(g_windowMgr, UIM_PUSHINTOCHATHISTORY, (int)msg, MSG_COLOR_INFO, 0, 0);
#endif

				RequestMove(x, y);

				g_pathOffset = i;

				break;
			}

			if (dist > 20)
			{
				proxy(UIWindowMgr)::SendMsg(g_windowMgr, UIM_PUSHINTOCHATHISTORY, (int)"I appear to be stuck...", MSG_COLOR_INFO, 0, 0);

				PickGoal();

				break;
			}
		}
	}
}

static void BeginIdleMode()
{
	g_isIdleMode = true;
	g_currentTarget = 0;

	g_pickInfo.reserve(25);

	proxy(UIWindowMgr)::SendMsg(g_windowMgr, UIM_PUSHINTOCHATHISTORY, (int)"Starting automatic play...", MSG_COLOR_INFO, 0, 0);
}

static bool IsIdleMode()
{
	return g_isIdleMode;
}

static void EndIdleMode()
{
	g_isIdleMode = false;
	g_mustRecalculatePath = true;
	g_goalX = 0;
	g_goalY = 0;

	g_pickInfo.clear();

	proxy(UIWindowMgr)::SendMsg(g_windowMgr, UIM_PUSHINTOCHATHISTORY, (int)"Stopped automatic play", MSG_COLOR_INFO, 0, 0);
}

static void PrintPickInfo()
{
	sprintf_s(msg, "| %-20s| %-20s| %-20s|", "GID", "Job", "ClassType");

	proxy(UIWindowMgr)::SendMsg(g_windowMgr, UIM_PUSHINTOCHATHISTORY, (int)msg, MSG_COLOR_INFO, 0, 0);

	for (auto i : g_pickInfo)
	{
		sprintf_s(msg, "| %-20d| %-20d| %-20d|", i.m_gid, i.m_job, i.m_classType);

		proxy(UIWindowMgr)::SendMsg(g_windowMgr, UIM_PUSHINTOCHATHISTORY, (int)msg, MSG_COLOR_INFO, 0, 0);
	}
}

static const CActorPickInfo& GetActorPickInfo(size_t GID)
{
	for (const auto &i : g_pickInfo)
	{
		if (i.m_gid == GID)
		{
			return i;
		}
	}

	throw 0;
}

static bool IsVisibleActor(size_t GID)
{
	for (const auto &i : g_pickInfo)
	{
		if (i.m_gid == GID)
		{
			return true;
		}
	}

	return false;
}

static void CancelTarget()
{
	if (g_currentTarget > 0)
	{
#if LOG_VERBOSE
		proxy(UIWindowMgr)::SendMsg(g_windowMgr, UIM_PUSHINTOCHATHISTORY, (int)"Cancelling target ...", MSG_COLOR_INFO, 0, 0);
#endif

		g_currentTarget = 0;
		g_mouseClick = false;

		g_mouse->m_btnState[0] = BTN_NONE;
		g_mouse->m_btnState[1] = BTN_NONE;
		g_mouse->m_oldBtnState[0] = BTN_NONE;
		g_mouse->m_oldBtnState[1] = BTN_NONE;

		g_mustRecalculatePath = true;
	}
}

static inline bool IsValidTarget()
{
	return g_currentTarget > 0 && IsVisibleActor(g_currentTarget);
}

static bool ProcessTarget()
{
	const auto &info = GetActorPickInfo(g_currentTarget);

	g_mouseX = (int)(info.m_vectors[0].x + info.m_vectors[1].x) / 2;
	g_mouseY = (int)(info.m_vectors[0].y + info.m_vectors[1].y) / 2;
	g_mouseClick = true;

	return true;
}

static bool FindTarget()
{
	CancelTarget();

	vector3d center;

	center.x = g_renderer->m_width / 2.f;
	center.y = g_renderer->m_height / 2.f;

	std::sort(g_pickInfo.begin(), g_pickInfo.end(), [center](const CActorPickInfo &left, const CActorPickInfo &right)
	{
		auto distToCenter = [center](const CActorPickInfo &v)
		{
			auto x = std::abs(center.x - (v.m_vectors[0].x + v.m_vectors[1].x) / 2.0f);
			auto y = std::abs(center.y - (v.m_vectors[0].y + v.m_vectors[1].y) / 2.0f);

			return std::sqrtf(x * x + y * y);
		};

		return distToCenter(right) > distToCenter(left);
	});

	for (auto i : g_pickInfo)
	{
		if (i.m_job > JT_MON_BEGIN && i.m_job < JT_2004_JOB_BEGIN && i.m_gid != 0xFFFFFFFF)
		{
			// TODO: Can we walk to target?

			g_currentTarget = i.m_gid;

#if LOG_VERBOSE
			sprintf_s(msg, "Found new target (GID=%d)", g_currentTarget);

			proxy(UIWindowMgr)::SendMsg(g_windowMgr, UIM_PUSHINTOCHATHISTORY, (int)msg, MSG_COLOR_INFO, 0, 0);
#endif

			return true;
		}
	}

	return false;
}

static void ProcessIdleMode()
{
	if (g_isIdleMode)
	{
		if (g_printPickInfo)
		{
			PrintPickInfo();
			g_printPickInfo = false;
		}

		if (IsValidTarget() || FindTarget())
		{
			ProcessTarget();
		}
		else
		{
			CancelTarget();
			FollowPlan();
		}

		g_pickInfo.clear();
	}
}

// Misc

int CheckSystemMessageHook()
{
	ProcessIdleMode();
	CheckSystemMessage();

	return TRUE;
}

// Mouse

void CMouseHook::ReadState()
{
	static int press_cycle = 0;

	if (IsIdleMode())
	{
		static int lastMouseX = m_xPos;
		static int lastMouseY = m_yPos;

		//if (g_mustUpdatePlayerPos)
		//{
		//	g_mouseX = g_renderer->m_width / 2;
		//	g_mouseY = g_renderer->m_height / 2 + 50;
		//
		//	press_cycle = 0;
		//
		//	g_mouseClick = true;
		//	g_mustUpdatePlayerPos = false;
		//}

		m_xPos = lastMouseX + (g_mouseX - lastMouseX) / 10;
		m_yPos = lastMouseY + (g_mouseY - lastMouseY) / 10;
		
		m_oldBtnState[0] = m_btnState[0];
		m_oldBtnState[1] = m_btnState[1];
		
		if (g_mouseClick)
		{
			if (press_cycle < 10)
			{
				m_btnState[0] = BTN_NONE;
			}
			else if (press_cycle == 11)
			{
				m_btnState[0] = BTN_DOWN;
			}
			else
			{
				m_btnState[0] = BTN_PRESSED;
			}

			press_cycle++;

			if (press_cycle >= 210)
			{
				press_cycle = 0;
				g_mouseClick = false;
			}
		}
		else
		{
			m_btnState[0] = BTN_NONE;
			press_cycle = 0;
		}
		
		m_btnState[1] = BTN_NONE;

		lastMouseX = m_xPos;
		lastMouseY = m_yPos;
	}
	else
	{
		proxy(CMouse)::ReadState(this);
	}
}

// ActorPickNode

void CActorPickNodeHook::AddPickInfo(struct CActorPickInfo *pickInfo)
{
	if (IsIdleMode())
	{
		g_pickInfo.push_back(*pickInfo);
	}

	proxy(CActorPickNode)::AddPickInfo(this, pickInfo);
}

// World

void CWorldHook::OnEnterFrame()
{
	auto gatname = g_mapName + ".gat";
	auto attr = (C3dAttr *)proxy(CResMgr)::Get(g_resMgr(), gatname.c_str());

	g_mapWidth = attr->m_width;
	g_mapHeight = attr->m_height;
	g_mapCells = std::vector<CAttrCell>(attr->cells, attr->cells + g_mapWidth * g_mapHeight);

	g_playerX = 0;
	g_playerY = 0;
	g_playerDestX = 0;
	g_playerDestY = 0;

	proxy(CWorld)::OnEnterFrame(this);

	g_mustUpdateMapColor = true;
}

// UIWindowMgr

void UIWindowMgrHook::MakeWindow(int windowId)
{
	proxy(UIWindowMgr)::MakeWindow(this, windowId);
}

UIWindow *UIWindowMgrHook::HitTest(int x, int y)
{
	if (IsIdleMode())
	{
		return nullptr;
	}

	return proxy(UIWindowMgr)::HitTest(this, x, y);
}

// CModeMgr

size_t CModeMgrHook::Switch(int modeType, char *modeName)
{
	//std::string msg{ "[Debug] CModeMgr::Switch (modeName = " };
	//
	//msg.append(modeName);
	//msg.append(")");
	//
	//proxy(UIWindowMgr)::SendMsg(g_windowMgr, UIM_PUSHINTOCHATHISTORY, (int)msg.c_str(), MSG_COLOR_INFO, 0, 0);

	srand(timeGetTime_());

	g_mapName = modeName;
	g_mapName = g_mapName.substr(0, g_mapName.size() - 4);

	g_mustUpdatePlayerPos = true;

	if (IsIdleMode())
	{
		EndIdleMode();
	}

	return proxy(CModeMgr)::Switch(this, modeType, modeName);
}

// CRenderer

size_t CRendererHook::DrawScene()
{
#ifndef DISABLE_MAPCOLORTABLE
	if (g_mustUpdateMapColor)
	{
		size_t color;

		sprintf_s(msg, "%s.rsw", g_mapName.c_str());

		if (GetMapColor(msg, color))
		{
			g_renderer->m_clearColor = color;
		}

		g_mustUpdateMapColor = false;
	}
#endif

	auto result = proxy(CRenderer)::DrawScene(this);

	if (IsIdleMode())
	{
		proxy(CRenderer)::TextOutScreen(this, m_width / 2 - 50 + 1, m_height / 2 - 95 + 1, (char *)"[AUTO MODE]", 0x000000);
		proxy(CRenderer)::TextOutScreen(this, m_width / 2 - 50 + 0, m_height / 2 - 95 + 0, (char *)"[AUTO MODE]", 0x00FFFF);
	}

	return result;
}

// CRagConnection

size_t CRagConnectionHook::OnSend(void)
{
	return proxy(CRagConnection)::OnSend(this);
}

size_t CRagConnectionHook::OnRecv(void)
{
	return proxy(CRagConnection)::OnRecv(this);
}

size_t CRagConnectionHook::RecvPacket(char *buf, short *size)
{
	auto result = proxy(CRagConnection)::RecvPacket(this, buf, size);

	if ((result & 1) && *(unsigned short *)buf == 0x87u)
	{
		struct MOVESTRUCT
		{
			unsigned int startTime;
			int sx;
			int sy;
			int cellX;
			int cellY;
			int dx;
			int dy;
		};

#pragma pack(push, 1)
		struct PACKET_ZC_NOTIFY_PLAYERMOVE {
			short PacketType;
			size_t moveStartTime;
			unsigned char MoveData[6];
		};
#pragma pack(pop)

		auto p = (PACKET_ZC_NOTIFY_PLAYERMOVE *)buf;
		MOVESTRUCT ms;

		ms.startTime = p->moveStartTime;

		DecodeSrcDst(p->MoveData, ms.sx, ms.sy, ms.dx, ms.dy, ms.cellX, ms.cellY);

		g_playerMoveStart = timeGetTime_();
		g_playerX = ms.sx;
		g_playerY = ms.sy;
		g_playerDestX = ms.dx;
		g_playerDestY = ms.dy;

		g_waitingForMove = false;
		g_mustUpdatePlayerPos = false;

		//sprintf_s(msg, "[Debug] moving (%d %d) -> (%d %d)", g_playerX, g_playerY, g_playerDestX, g_playerDestY);
		//
		//proxy(UIWindowMgr)::SendMsg(g_windowMgr, UIM_PUSHINTOCHATHISTORY, (int)msg, MSG_COLOR_INFO, 0, 0);

	}

	//if (result & 1)
	//{
	//	sprintf_s(msg, "[Debug] CRagConnection::RecvPacket %04x (%d bytes)", (size_t)*(unsigned short *)buf, size == nullptr ? -1 : *size);
	//	
	//	proxy(UIWindowMgr)::SendMsg(g_windowMgr, UIM_PUSHINTOCHATHISTORY, (int)msg, MSG_COLOR_INFO, 0, 0);
	//}

	return result;
}

size_t CRagConnectionHook::SendPacket(size_t size, char *buf)
{
	//sprintf_s(msg, "[Debug] CRagConnection::SendPacket %04x (%d bytes)", (size_t)*(unsigned short *)buf, size);
	//
	//proxy(UIWindowMgr)::SendMsg(g_windowMgr, UIM_PUSHINTOCHATHISTORY, (int)msg, MSG_COLOR_INFO, 0, 0);

	return proxy(CRagConnection)::SendPacket(this, size, buf);
}

// CSession

int CSessionHook::GetTalkType(const char *chatBuf, enum TALKTYPE *talkType, void *param)
{
#ifndef DISABLE_IDLE_MODE
	if (_strcmpi("/idle", chatBuf) == 0 ||
		_strcmpi("/auto", chatBuf) == 0)
	{
		if (IsIdleMode())
		{
			EndIdleMode();
		}
		else
		{
			if (g_mustUpdatePlayerPos)
			{
				proxy(UIWindowMgr)::SendMsg(g_windowMgr, UIM_PUSHINTOCHATHISTORY, (int)"Please walk one tile...", MSG_COLOR_ERROR, 0, 0);
			}
			else
			{
				BeginIdleMode();
			}
		}

		*talkType = TT_NORMAL;

		return -1;
	}

	if (_strcmpi("/pickinfo", chatBuf) == 0)
	{
		if (IsIdleMode())
		{
			g_printPickInfo = true;
		}
		else
		{
			proxy(UIWindowMgr)::SendMsg(g_windowMgr, UIM_PUSHINTOCHATHISTORY, (int)"Not tracking pickinfo!", MSG_COLOR_ERROR, 0, 0);
		}

		*talkType = TT_NORMAL;

		return -1;
	}
#endif

	//if (chatBuf != nullptr)
	//{
	//	sprintf_s(msg, "[Debug] CSession::GetTalkType \"%s\"", chatBuf);
	//	
	//	proxy(UIWindowMgr)::SendMsg(g_windowMgr, UIM_PUSHINTOCHATHISTORY, (int)msg, MSG_COLOR_INFO, 0, 0);
	//}

	return proxy(CSession)::GetTalkType(this, chatBuf, talkType, param);
}
