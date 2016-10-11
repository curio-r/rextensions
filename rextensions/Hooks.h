#pragma once

#include "ProxyImpl.h"
#include "enum.h"

#define HOOK_ERROR "Hook class has a virtual function table"

// Hooks for their native counterparts

int CheckSystemMessageHook();

struct CActorPickNodeHook : CActorPickNode
{
	void AddPickInfo(struct CActorPickInfo *pickInfo);
};

struct CMouseHook : CMouse
{
	void ReadState();
};

struct CWorldHook : CWorld
{
	void OnEnterFrame();
};

struct UIWindowMgrHook : UIWindowMgr
{
	void MakeWindow(int windowId);
	UIWindow *HitTest(int x, int y);
};

struct CModeMgrHook : CModeMgr
{
	size_t Switch(int modeType, char *modeName);
};

struct CRendererHook : CRenderer
{
	size_t DrawScene();
};

struct CRagConnectionHook : CRagConnection
{
	size_t OnSend(void);
	size_t SendPacket(size_t size, char *buf);
	size_t OnRecv(void);
	size_t RecvPacket(char *buf, short *size);
};

struct CSessionHook : CSession
{
	int GetTalkType(const char *chatBuf, enum TALKTYPE *talkType, void *param);
};

static_assert(!std::is_polymorphic<struct CRendererHook>::value, HOOK_ERROR);
static_assert(!std::is_polymorphic<struct CModeMgrHook>::value, HOOK_ERROR);
static_assert(!std::is_polymorphic<struct UIWindowMgrHook>::value, HOOK_ERROR);
static_assert(!std::is_polymorphic<struct CRagConnectionHook>::value, HOOK_ERROR);
static_assert(!std::is_polymorphic<struct CSessionHook>::value, HOOK_ERROR);
