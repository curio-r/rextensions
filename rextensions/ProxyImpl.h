#pragma once
#include "Proxy.h"
#include "structs.h"

#define proxy(CLASS) CLASS##Proxy
#define define_proxy(CLASS) struct proxy(CLASS) : public ClassProxy<CLASS>

extern int(*CheckSystemMessage)();

// Defines proxy classes for accessing native classes through pointers

extern struct proxy(UIWindowMgr) g_windowMgr;
extern struct proxy(CRenderer) g_renderer;
extern struct proxy(CModeMgr) g_modeMgr;
extern struct proxy(CRagConnection) g_connection;
extern struct proxy(CSession) g_session;
extern struct proxy(CMouse) g_mouse;

struct proxy(UIWindowMgr) : public ClassProxy<UIWindowMgr>
{
	static MethodRef<void *(__thiscall UIWindowMgr::*)(int windowId)> MakeWindow;
	static MethodRef<UIWindow *(__thiscall UIWindowMgr::*)(int x, int y)> HitTest;
	static MethodRef<size_t *(__thiscall UIWindowMgr::*)(int message, int val1, int val2, int val3, int val4)> SendMsg;

	inline proxy(UIWindowMgr) & operator=(UIWindowMgr * Instance)
	{
		_Instance = Instance;

		return *this;
	}
};

struct proxy(CRenderer) : public ClassRefProxy<CRenderer>
{
	static MethodRef<size_t(__thiscall CRenderer::*)(void)> DrawScene;
	static MethodRef<size_t(__thiscall CRenderer::*)(int x, int y, char *text, unsigned int colorRef)> TextOutScreen;

	inline proxy(CRenderer) & operator=(CRenderer ** Instance)
	{
		_Instance = Instance;

		return *this;
	}
};

struct proxy(CMouse) : public ClassProxy<CMouse>
{
	static MethodRef<void(CMouse::*)(void)> ReadState;

	inline proxy(CMouse) & operator=(CMouse * Instance)
	{
		_Instance = Instance;

		return *this;
	}
};

struct proxy(CGameMode) : public ClassProxy<CGameMode>
{
	static MethodRef<void(CGameMode::*)(size_t actorGid, int attackOnce)> ProceedAttack;
};

struct proxy(CModeMgr) : public ClassProxy<CModeMgr>
{
	static MethodRef<size_t(CModeMgr::*)(int modeType, char *modeName)> Switch;

	inline proxy(CModeMgr) & operator=(CModeMgr * Instance)
	{
		_Instance = Instance;

		return *this;
	}
};

struct proxy(CActorPickNode) : public ClassProxy<CActorPickNode>
{
	static MethodRef<void(CActorPickNode::*)(CActorPickInfo *pickInfo)> AddPickInfo;

	inline proxy(CActorPickNode) & operator=(CActorPickNode * Instance)
	{
		_Instance = Instance;

		return *this;
	}
};

struct proxy(CSession) : public ClassProxy<CSession>
{
	static MethodRef<int(CSession::*)(const char *chatBuf, enum TALKTYPE *talkType, void *param)> GetTalkType;

	inline proxy(CSession) & operator=(CSession * Instance)
	{
		_Instance = Instance;

		return *this;
	}
};

extern CResMgr *(__cdecl *g_resMgr)();

struct proxy(CResMgr) : public ClassProxy<CResMgr>
{
	static MethodRef<void *(CResMgr::*)(char const *fNameInput)> Get;
};

struct proxy(CWorld) : public ClassProxy<CWorld>
{
	static MethodRef<void(CWorld::*)(void)> OnEnterFrame;
};

struct proxy(CRagConnection) : public ClassProxy<CRagConnection>
{
	static MethodRef<size_t(CRagConnection::*)(void)> OnSend;
	static MethodRef<size_t(CRagConnection::*)(size_t size, char *buf)> SendPacket;

	static MethodRef<size_t(CRagConnection::*)(void)> OnRecv;
	static MethodRef<size_t(CRagConnection::*)(char *buf, short *size)> RecvPacket;

	static CRagConnection* (__cdecl *instanceR)();

	static int(__stdcall *s_wsSend)(size_t, size_t, size_t, size_t);
	static int(__stdcall *s_wsRecv)(size_t, size_t, size_t, size_t);

	inline proxy(CRagConnection) & operator=(CRagConnection * Instance)
	{
		_Instance = Instance;

		return *this;
	}
};
