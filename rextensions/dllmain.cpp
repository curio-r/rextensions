#include "stdafx.h"
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <functional>

#include "ProcMem.h"
#include "Composer.h"
#include "Proxy.h"

#include "structs.h"
#include "overrides.h"

#define PROC_HOOK(NAME) GLOBAL_HOOK_##NAME

BOOL CALLBACK ImportFileCallback(
	PVOID pContext,
	HMODULE nOrdinal,
	PCSTR pszName
	)
{
	if (pszName != nullptr)
	{
		printf(pszName);
	}

	return TRUE;
}

void ErrorMsg(std::string msg)
{
	MessageBoxA(NULL, msg.c_str(), "Error", 0);
}

struct UIWindowMgrProxy : public ClassProxy<UIWindowMgr>
{
	static MethodRef<void *(__thiscall UIWindowMgr::*)(int windowId)> MakeWindow;

	UIWindowMgrProxy& operator=(UIWindowMgr * Instance)
	{
		_Instance = Instance;

		return *this;
	}
} g_windowMgr;

UIWindowMgrProxy::MethodRef<void *(__thiscall UIWindowMgr::*)(int windowId)> UIWindowMgrProxy::MakeWindow;

struct CRendererProxy : public ClassRefProxy<CRenderer>
{
	static MethodRef<size_t(__thiscall CRenderer::*)(void)> DrawScene;
	static MethodRef<size_t(__thiscall CRenderer::*)(int x, int y, char *text, unsigned int colorRef)> TextOutScreen;

	CRendererProxy& operator=(CRenderer ** Instance)
	{
		_Instance = Instance;

		return *this;
	}
} g_renderer;

CRendererProxy::MethodRef<size_t(__thiscall CRenderer::*)(void)> CRendererProxy::DrawScene;
CRendererProxy::MethodRef<size_t(__thiscall CRenderer::*)(int x, int y, char *text, unsigned int colorRef)> CRendererProxy::TextOutScreen;

struct CModeMgrProxy : public ClassProxy<CModeMgr>
{
	static MethodRef<size_t (CModeMgr::*)(int modeType, char *modeName)> Switch;
	
	CModeMgrProxy& operator=(CModeMgr * Instance)
	{
		_Instance = Instance;

		return *this;
	}
} g_modeMgr;

CModeMgrProxy::MethodRef<size_t(CModeMgr::*)(int modeType, char *modeName)> CModeMgrProxy::Switch;

struct UIWindowMgrHook : UIWindowMgr
{
	void MakeWindow(int windowId)
	{
		UIWindowMgrProxy::MakeWindow(this, windowId);
	}
};

struct CModeMgrHook : CModeMgr
{
	size_t Switch(int modeType, char *modeName)
	{
		return CModeMgrProxy::Switch(this, modeType, modeName);
	}
};

struct CRendererHook : CRenderer
{
	size_t DrawScene()
	{
		auto result = CRendererProxy::DrawScene(this);

		CRendererProxy::TextOutScreen(this, this->m_width - 108 + 1, this->m_height - 20 + 1, (char *)"Curiosity Client", 0x000000);
		CRendererProxy::TextOutScreen(this, this->m_width - 108 + 0, this->m_height - 20 + 0, (char *)"Curiosity Client", 0xFFFFFF);

		return result;
	}
};

static_assert(!std::is_polymorphic<struct CRendererHook>::value, "Struct has virtual function table");
static_assert(!std::is_polymorphic<struct CModeMgrHook>::value, "Struct has virtual function table");
static_assert(!std::is_polymorphic<struct UIWindowMgrHook>::value, "Struct has virtual function table");

BOOL(*CheckSystemMessage)();

class ClientGlobals
{
public:

	class BinarySearchComposer *m_client;

public:

	ClientGlobals(class BinarySearchComposer *composer)
		: m_client(composer)
	{
		if (!FindWindowMgr())
		{
			ErrorMsg("Unable to find CWindowMgr instance");
		}

		if (!FindCheckSystemMessage())
		{
			ErrorMsg("Unable to find CheckSystemMessage");
		}

		if (!FindRenderer())
		{
			ErrorMsg("Unable to find renderer");
		}

		if (!FindModeMgr())
		{
			ErrorMsg("Unable to find mode manager");
		}
	}

	bool FindModeMgr()
	{
		auto addr = m_client->FromCode({ 0x8B, 0xCC, 0x04 }, true) // mov ?, 4h
			.FindBelow({ 0x8D, 0xCC, 0x30 }, 50, true) // lea ?, [? + 30h]
			.FindBelow({ 0x89, 0xCC, 0x5C }, 50, true) // mov [? + 5Ch], ?
			.FindBelow({ 0xC2, 0x08, 0x00 }, 50, false) // retn 8h
			.FindStartOfFunction()
			.GetOne();

		if (addr == nullptr)
		{
			return false;
		}

		g_modeMgr.Switch = addr;

		auto addr2 = m_client->FromCallTo(addr)
			.FindAbove({ 0xB9 }, 5, false) // mov ecx, ?
			.GetMostCommon();

		g_modeMgr = (CModeMgr *)((void *)*(DWORD *)(addr2 + 1));

		return true;
	}

	bool FindRenderer()
	{
		auto addr = m_client->FromString("Sunday")
			.FindReferences()
			.FindBelow({ 0x8B, 0x51, 0x28 }, 1000) // mov edx, [ecx + 28h]
			.FindAbove({ 0x8B, 0x0D }) // mov ecx, ?
			.GetOne();

		if (addr == nullptr)
		{
			return false;
		}

		g_renderer = (CRenderer**)(*(size_t *)(addr + 2));

		// TextOutScreen

		auto addr2 = m_client->FromOffset(addr).FindBelow({ 0xE8 }, 50).GetOne(); // call ?

		if (addr2 == nullptr)
		{
			return false;
		}

		CRendererProxy::TextOutScreen = (void *)ProcMem::DecodeCallAddress(addr2);

		// DrawScene

		auto addr3 = m_client->FromOffset(g_renderer)
			.FindReferences()
			.FindAbove({ 0x8B, 0x0D }, 5) // mov ecx, g_renderer
			.FindBelow({ 0xE8, 0xCC, 0xCC, 0xCC, 0xCC, 0x84, 0xC0, 0x74 }, 20, true) // call ?; test al, al; jz ?;
			.GetOne();

		if (addr3 == nullptr)
		{
			return false;
		}

		CRendererProxy::DrawScene = (void *)ProcMem::DecodeCallAddress(addr3);

		return true;
	}

	bool FindCheckSystemMessage()
	{
		CheckSystemMessage = (decltype(CheckSystemMessage))m_client->FromImport("user32.dll", "TranslateMessage")
			.FindReferences()
			.FindAbove({ 0x6A, 0x01, 0x6A, 0x00, 0x6A, 0x00, 0x6A, 0x00 })
			.FindStartOfFunction()
			.GetOne();

		return CheckSystemMessage != nullptr;
	}

	bool FindWindowMgr()
	{
		auto q = m_client->FromString("PingLog.txt")
			.FindReferences()
			.FindAbove({ 0x6A, 0x01, 0x6A, 0x11, 0x6a, 0x79 }) // push 1h; push 11h; push 79h
			.FindAbove({ 0x6A, 0x06, 0xB9 }); // push 6h; mov ecx ?
		
		auto va = q.GetOne();

		if (va == nullptr)
		{
			return false;
		}
		
		g_windowMgr = (UIWindowMgr *)((void *)*(DWORD *)(va + 3));

		auto va2 = q.FindBelow({ 0xE8 }).GetOne(); // call ?

		if (va2 == nullptr)
		{
			return false;
		}

		g_windowMgr.MakeWindow = (void *)ProcMem::DecodeCallAddress(va2);

		return true;
	}
};

BOOL PROC_HOOK(CheckSystemMessage)()
{
	CheckSystemMessage();

	return TRUE;
}

#define AttachHook(PTR, FN) _AttachHook(PTR, FN, #FN)
template<typename T1, typename T2>
bool _AttachHook(T1 &&_Ptr, T2 &&_Fn, const std::string &_Name)
{
	if (DetourAttach((PVOID *)&_Ptr, (void *)_Fn) != NO_ERROR)
	{
		ErrorMsg("Error while processing proc for " + _Name);

		return false;
	}

	return true;
}

bool InstallHooks()
{
	DetourTransactionBegin();

	if (AttachHook(CheckSystemMessage, PROC_HOOK(CheckSystemMessage)) &&
		AttachHook(CRendererProxy::DrawScene, void_cast(&CRendererHook::DrawScene)) &&
		AttachHook(CModeMgrProxy::Switch, void_cast(&CModeMgrHook::Switch)) &&
		AttachHook(UIWindowMgrProxy::MakeWindow, void_cast(&UIWindowMgrHook::MakeWindow)))
	{
		DetourTransactionCommit();
		
		return true;
	}

	DetourTransactionAbort();

	return false;
}

ProcMem *g_procFinder;
BinarySearchComposer *g_composer;
ClientGlobals *g_client;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		{
			g_procFinder = new ProcMem(GetModuleHandle(NULL));
			g_composer = new BinarySearchComposer(g_procFinder);
			g_client = new ClientGlobals(g_composer);

			InstallHooks();
		}
		
		break;

	case DLL_PROCESS_DETACH:

		delete g_procFinder;
		delete g_composer;
		delete g_client;

		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:

		break;
	}

	return TRUE;
}

