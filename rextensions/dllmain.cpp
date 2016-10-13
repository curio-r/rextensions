#include "stdafx.h"
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <functional>

#include "ProcMem.h"
#include "Composer.h"
#include "ProxyImpl.h"
#include "Hooks.h"
#include "MapColorTable.h"

#define PROC_HOOK(NAME) GLOBAL_HOOK_##NAME

void ErrorMsg(std::string msg)
{
	MessageBoxA(NULL, msg.c_str(), "Error", 0);
}

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

		if (!FindConnection())
		{
			ErrorMsg("Unable to find connection");
		}

		if (!FindSession())
		{
			ErrorMsg("Unable to find talktype");
		}

		if (!FindWorldAndResMgr())
		{
			ErrorMsg("Unable to find world");
		}

		if (!FindPickNode())
		{
			ErrorMsg("Can't find pick node");
		}

		if (!FindGameMode())
		{
			ErrorMsg("Can't find game mode");
		}

		if (!FindMouse())
		{
			ErrorMsg("Can't find mouse");
		}

		if (!FindCFile())
		{
			ErrorMsg("Can't find CFile");
		}
	}

	bool FindCFile()
	{
		auto query = m_client->FromString("%s%s.gr2")
			.FindReferences()
			.FindBelow({
				0xE8, 0xCC, 0xCC, 0xCC, 0xCC, // call ? (CFile::Open)
				0x84, 0xC0 // test al, al
			}, 200, true);

		if (query.Count() == 0)
		{
			return false;
		}

		proxy(CFile)::Open = ProcMem::DecodeCallAddress(query.GetOne());

		// TOOD: Locate CFile::Close

		return true;
	}

	bool FindMouse()
	{
		// Very bad check, can easily be wrong!!

		auto query = m_client->FromCallTo((BYTE *)CheckSystemMessage)
			.FindBelow({ 0xE8 })
			.If().FindBelow({ 
				0xB9, 0xCC, 0xCC, 0xCC, 0xCC, // mov ecx, offset ? (g_mouse)
				0xE8 // call ? (CMouse::ReadState)
			}, 11, true);

		g_mouse = (CMouse *)*(CMouse**)(query.GetOne() + 6);

		proxy(CMouse)::ReadState = ProcMem::DecodeCallAddress(query.GetOne() + 10);

		return true;
	}

	bool FindGameMode()
	{
		return true;
	}

	bool FindPickNode()
	{
		auto AddInfo = m_client->FromCode({ 
				0xD1, 0xF9, // sar ecx, 1
				0xD1, 0xF8, // sar eax, 1
				0x3B, 0xF9 // cmp edi, ecx
			})
			.FindStartOfFunctionByCall(0x10, 3, 70);

		if (AddInfo.Count() != 1)
		{
			return false;
		}

		proxy(CActorPickNode)::AddPickInfo = AddInfo.GetOne();

		return true;
	}

	bool FindWorldAndResMgr()
	{
		char bytes[] = { 0x77u, 0x6Fu, 0x72u, 0x6Cu, 0x64u, 0xBFu, 0xA1u, 0x20u,
			0x4Eu, 0x55u, 0x4Cu, 0x4Cu, 0xB0u, 0xAAu, 0xC0u, 0xCCu, 0x00u };

		auto OnEnterFrame = m_client->FromString(bytes)
			.FindReferences()
			.FindStartOfFunction();

		if (OnEnterFrame.Count() == 0)
		{
			return false;
		}

		proxy(CWorld)::OnEnterFrame = OnEnterFrame.GetOne();

		auto ref_resmgr = m_client->FromString(bytes)
			.FindReferences()
			.First()
			.FindAbove({
				0xE8, 0xCC, 0xCC, 0xCC, 0xCC, // call ? (g_resMgr)
				0x8B, 0xC8, // mov ecx, eax (this = g_resMgr())
				0xE8 // call ? (CResMgr::Get)
			}, 100, true);

		if (ref_resmgr.Count() == 0)
		{
			return false;
		}

		g_resMgr = (decltype(g_resMgr))ProcMem::DecodeCallAddress(ref_resmgr.GetOne());

		proxy(CResMgr)::Get = ProcMem::DecodeCallAddress(ref_resmgr.GetOne() + 7);

		return true;
	}

	bool FindSession()
	{
		char bytes[] = { 0x2Fu, 0xA4u, 0xD3u, 0xA4u, 0xD0u, 0x00u };

		auto GetTalkType = m_client->FromString(bytes)
			.FindReferences()
			.FindStartOfFunction(3, 0, 500)
			.GetOne();

		if (GetTalkType == nullptr)
		{
			return false;
		}

		proxy(CSession)::GetTalkType = GetTalkType;

		auto ref_mov = m_client->FromCallTo(GetTalkType)
			.FindAbove({ 0xB9 }, 5); // mov ecx, ?

		g_session = (CSession *)*(size_t *)(ref_mov.GetMostCommonBytes(4) + 1);

		return true;
	}

	bool FindConnection()
	{
		// Find CConnection::s_wsSend, CConnection::s_wsRecv
		
		auto q = m_client->FromString("ws2_32.dll")
			.FindReferences()
			.FindBelow({ 0xFF, 0x15 }, 10) // call ? (LoadLibraryA)
			.Unless().FindReferencesToImportBelow("KERNEL32.dll", "FreeLibrary")
			.First()
			.FindBelow({ 0xA3 }); // mov ?, eax
		
		if (q.Count() < 3)
		{
			return false;
		}

		proxy(CRagConnection)::s_wsSend = (decltype(proxy(CRagConnection)::s_wsSend))*(BYTE **)(q.GetNth(2) + 1);
		proxy(CRagConnection)::s_wsRecv = (decltype(proxy(CRagConnection)::s_wsRecv))*(BYTE **)(q.GetNth(3) + 1);
		
		// Find CConnection::OnSend

		auto OnSend = m_client->FromOffset(proxy(CRagConnection)::s_wsSend)
			.FindReferences()
			.First()
			.FindStartOfFunction()
			.GetOne();

		proxy(CRagConnection)::OnSend = OnSend;

		// Find CRagConnection::SendPacket

		proxy(CRagConnection)::SendPacket = m_client->FromCallTo(OnSend)
			.FindStartOfFunction(100)
			.GetOne();

		// Find CRagConnection::RecvPacket

		auto q2 = m_client->FromCallTo((BYTE *)CheckSystemMessage) // CLoginMode::OnUpdate
			.FindBelow({ 0x8B, 0xCE }, 8) // mov ecx, esi
			.FindBelow({ 0xE8 }, 3) // call ? (CLoginMode::PollNetworkState)
			.FollowCall()
			.FindBelow({ 
				0x8B, 0xC8, // mov ecx, eax
				0xE8, 0xCC, 0xCC, 0xCC, 0xCC, // call ? (CRagConnection::RecvPacket)
				0x84, 0xC0, // cmp al, al
				0x0F  // jz ?
			}, 300, true);

		proxy(CRagConnection)::RecvPacket = (void *)ProcMem::DecodeCallAddress(q2.GetOne() + 2);

		// CRagConnection::instanceR is now right above

		proxy(CRagConnection)::instanceR = (decltype(proxy(CRagConnection)::instanceR))ProcMem::DecodeCallAddress(q2.FindAbove({ 0xE8 }).GetOne());
		
		return true;
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
			.GetMostCommonCall();

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

		proxy(CRenderer)::TextOutScreen = (void *)ProcMem::DecodeCallAddress(addr2);

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

		proxy(CRenderer)::DrawScene = (void *)ProcMem::DecodeCallAddress(addr3);

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

#define BYTES_OF(PTR) ((BYTE *)&(void *)PTR)[0], ((BYTE *)&(void *)PTR)[1], ((BYTE *)&(void *)PTR)[2], ((BYTE *)&(void *)PTR)[3]

		void * Ptr = g_windowMgr;

		auto query2 = m_client->FromString(" is Saved.")
			.FindReferences()
			.FindBelow({ 0xB9, BYTES_OF(Ptr) }) // mov ecx, offset g_windowMgr
			.FindBelow({ 0xE8 }); // call ?

		g_windowMgr.SendMsg = (void *)ProcMem::DecodeCallAddress(query2.GetMostCommonCall());


		// UIWindowMgr::Drop

		auto query3 = m_client->FromCode({ 0x68, 0xB2, 0x02, 0x00, 0x00 }) // CModeMgr::SendMsg+?
			.FindAbove({ 
				0xB9, BYTES_OF(Ptr), // mov eax, offset g_windowMgr
				0xE8, 0xCC, 0xCC, 0xCC, 0xCC, // call ? (UIWindowMgr::Drop)
				0x84, 0xC0 // test al, al
			}, 200, true);

		if (query3.Count() == 0)
		{
			return false;
		}

		auto Drop = (BYTE *)ProcMem::DecodeCallAddress(query3.GetOne() + 5);

		auto ref_call = m_client->FromOffset(Drop)
			.FindBelow({ 0xE8 })
			.GetOne();

		if (ref_call == nullptr)
		{
			return false;
		}
		
		proxy(UIWindowMgr)::HitTest = ProcMem::DecodeCallAddress(ref_call);

		return true;
	}
};

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

#define DetachHook(PTR, FN) _AttachHook(PTR, FN)
template<typename T1, typename T2>
bool DetachHook(T1 &&_Ptr, T2 &&_Fn)
{
	return DetourDetach((PVOID *)&_Ptr, (void *)_Fn) == NO_ERROR;
}

#define AttachMethodHook(CLASS, METHOD) AttachHook(proxy(CLASS) :: METHOD, void_cast(& CLASS##Hook :: METHOD))
#define DetachMethodHook(CLASS, METHOD) DetachHook(proxy(CLASS) :: METHOD, void_cast(& CLASS##Hook :: METHOD))

bool InstallHooks()
{
	DetourTransactionBegin();

	if (AttachHook(CheckSystemMessage, CheckSystemMessageHook) &&
		AttachMethodHook(CRenderer, DrawScene) &&
		AttachMethodHook(CModeMgr, Switch) &&
		AttachMethodHook(UIWindowMgr, MakeWindow) &&
		AttachMethodHook(CRagConnection, SendPacket) &&
		AttachMethodHook(CRagConnection, RecvPacket) &&
		AttachMethodHook(CSession, GetTalkType) &&
		AttachMethodHook(CWorld, OnEnterFrame) &&
		AttachMethodHook(CActorPickNode, AddPickInfo) &&
		AttachMethodHook(UIWindowMgr, HitTest) &&
		AttachMethodHook(CMouse, ReadState)
	) {
		DetourTransactionCommit();
		
		return true;
	}

	DetourTransactionAbort();

	return false;
}

bool RemoveHooks()
{
	DetourTransactionBegin();

	if (DetachHook(CheckSystemMessage, CheckSystemMessageHook) &&
		DetachMethodHook(CRenderer, DrawScene) &&
		DetachMethodHook(CModeMgr, Switch) &&
		DetachMethodHook(UIWindowMgr, MakeWindow) &&
		DetachMethodHook(CRagConnection, SendPacket) &&
		DetachMethodHook(CRagConnection, RecvPacket) &&
		DetachMethodHook(CSession, GetTalkType) &&
		DetachMethodHook(CWorld, OnEnterFrame) &&
		DetachMethodHook(CActorPickNode, AddPickInfo) &&
		DetachMethodHook(UIWindowMgr, HitTest) &&
		DetachMethodHook(CMouse, ReadState)
		) 
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
bool g_bHooksInstalled;

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

			g_bHooksInstalled = InstallHooks();

			if (g_bHooksInstalled)
			{
				InitMapColorTable();
			}
		}
		
		break;

	case DLL_PROCESS_DETACH:

		delete g_procFinder;
		delete g_composer;
		delete g_client;

		if (g_bHooksInstalled)
		{
			RemoveHooks();
		}

		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:

		break;
	}

	return TRUE;
}
