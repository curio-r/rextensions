#include "stdafx.h"
#include "ProxyImpl.h"

BOOL(*CheckSystemMessage)() = nullptr;

proxy(UIWindowMgr)::MethodRef<void *(__thiscall UIWindowMgr::*)(int windowId)> proxy(UIWindowMgr)::MakeWindow;
proxy(UIWindowMgr)::MethodRef<UIWindow *(__thiscall UIWindowMgr::*)(int x, int y)> proxy(UIWindowMgr)::HitTest;
proxy(UIWindowMgr)::MethodRef<size_t *(__thiscall UIWindowMgr::*)(int message, int val1, int val2, int val3, int val4)> proxy(UIWindowMgr)::SendMsg;
proxy(CRenderer)::MethodRef<size_t(__thiscall CRenderer::*)(void)> proxy(CRenderer)::DrawScene;
proxy(CRenderer)::MethodRef<size_t(__thiscall CRenderer::*)(int x, int y, char *text, unsigned int colorRef)> proxy(CRenderer)::TextOutScreen;
proxy(CModeMgr)::MethodRef<size_t(CModeMgr::*)(int modeType, char *modeName)> proxy(CModeMgr)::Switch;
proxy(CRagConnection)::MethodRef<size_t(CRagConnection::*)(void)> proxy(CRagConnection)::OnSend;
proxy(CRagConnection)::MethodRef<size_t(CRagConnection::*)(size_t size, char *buf)> proxy(CRagConnection)::SendPacket;
proxy(CRagConnection)::MethodRef<size_t(CRagConnection::*)(void)> proxy(CRagConnection)::OnRecv;
proxy(CRagConnection)::MethodRef<size_t(CRagConnection::*)(char *buf, short *size)> proxy(CRagConnection)::RecvPacket;
proxy(CSession)::MethodRef<int(CSession::*)(const char *chatBuf, enum TALKTYPE *talkType, void *param)> proxy(CSession)::GetTalkType;
proxy(CWorld)::MethodRef<void(CWorld::*)(void)> proxy(CWorld)::OnEnterFrame;
proxy(CMouse)::MethodRef<void(CMouse::*)(void)> proxy(CMouse)::ReadState;
proxy(CResMgr)::MethodRef<void *(CResMgr::*)(char const *fNameInput)> proxy(CResMgr)::Get;
CResMgr *(__cdecl *g_resMgr)() = nullptr;

proxy(CFile)::MethodRef<bool (CFile::*)(const char *fName, unsigned long nOpenFlags)> proxy(CFile)::Open;
proxy(CFile)::MethodRef<void (CFile::*)()> proxy(CFile)::Close;
proxy(CFile)::MethodRef<bool (CFile::*)(void *lpBuf, unsigned long nCount)> proxy(CFile)::Read;

proxy(CActorPickNode)::MethodRef<void(CActorPickNode::*)(CActorPickInfo *pickInfo)> proxy(CActorPickNode)::AddPickInfo;
proxy(CGameMode)::MethodRef<void(CGameMode::*)(size_t actorGid, int attackOnce)> proxy(CGameMode)::ProceedAttack;

CRagConnection* (__cdecl *proxy(CRagConnection)::instanceR)();
int(__stdcall *proxy(CRagConnection)::s_wsSend)(size_t, size_t, size_t, size_t);
int(__stdcall *proxy(CRagConnection)::s_wsRecv)(size_t, size_t, size_t, size_t);

proxy(UIWindowMgr) g_windowMgr;
proxy(CRenderer) g_renderer;
proxy(CModeMgr) g_modeMgr;
proxy(CRagConnection) g_connection;
proxy(CSession) g_session;
proxy(CMouse) g_mouse;
