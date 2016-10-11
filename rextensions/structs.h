#pragma once
#include <list>
#include <map>
#include <vector>
#include "enum.h"

struct CAttrCell
{
	float h1;
	float h2;
	float h3;
	float h4;
	int flag;
};

struct C3dAttr
{
	size_t dummy[68];
	size_t m_width;
	size_t m_height;
	size_t dword_dummy0;
	size_t dword_dummy2;
	CAttrCell *cells;
};

struct CMouse
{
	void *m_lpdi;
	void *m_pMouse;
	void *m_hevtMouse;
	int m_xDelta;
	int m_yDelta;
	int m_xPos;
	int m_yPos;
	size_t m_wheel;
	EBtnState m_oldBtnState[2];
	EBtnState m_btnState[2];
	size_t m_dblclkCnt[3];
	size_t m_dblclkTime;
	size_t m_bSwapButton;
};

typedef struct vector3d
{
	float x, y, z;
} vector3d;

struct CGameMode
{
	size_t dword_0;
	size_t dword_4;
	size_t dword_8;
	size_t dword_C;
	size_t dword_10;
	size_t dword_14;
	size_t dword_18;
	size_t dword_1C;
	size_t dword_20;
};

struct CActorPickInfo
{
	vector3d m_vectors[2];
	unsigned int m_gid;
	int m_job;
	int m_classType;
	int m_isPkState;
};

struct CActorPickNode
{
	size_t dword_0;
	size_t dword_4;
	size_t dword_8;
	size_t dword_C;
	size_t dword_10;
	size_t dword_14;
	size_t dword_18;
	size_t dword_1C;
	size_t dword_20;
};

struct CResMgr
{
	size_t dword_0;
	size_t dword_4;
	size_t dword_8;
	size_t dword_C;
	size_t dword_10;
	size_t dword_14;
	size_t dword_18;
	size_t dword_1C;
	size_t dword_20;
};

struct CWorld
{
	size_t dword_0;
	size_t dword_4;
	size_t dword_8;
	size_t dword_C;
	size_t dword_10;
	size_t dword_14;
	size_t dword_18;
	size_t dword_1C;
	size_t dword_20;
};

struct CSession
{
	size_t dword_0;
	size_t dword_4;
	size_t dword_8;
	size_t dword_C;
	size_t dword_10;
	size_t dword_14;
	size_t dword_18;
	size_t dword_1C;
	size_t dword_20;
};

struct CRagConnection
{
	size_t dword_0;
	size_t dword_4;
	size_t dword_8;
	size_t dword_C;
	size_t dword_10;
	size_t dword_14;
	size_t dword_18;
	size_t dword_1C;
	size_t dword_20;
};

struct CModeMgr
{
	size_t dword_0;
	size_t dword_4;
	size_t dword_8;
	size_t dword_C;
	size_t dword_10;
	size_t dword_14;
	size_t dword_18;
	size_t dword_1C;
	size_t dword_20;
};

struct CRenderer
{
	size_t dword_0;
	size_t dword_4;
	size_t dword_8;
	size_t dword_C;
	size_t dword_10;
	size_t dword_14;
	size_t dword_18;
	size_t dword_1C;
	size_t dword_20;
	size_t m_width;
	size_t m_height;
	size_t dword_2C;
	size_t dword_30;
	size_t dword_34;
	size_t dword_38;
	size_t dword_3C;
	size_t dword_40;
	size_t dword_44;
	size_t dword_48;
	size_t dword_4C;
	size_t dword_50;
	size_t dword_54;
	size_t dword_58;
	size_t dword_5C;
	size_t dword_60;
	size_t dword_64;
	size_t dword_68;
	size_t dword_6C;
	size_t dword_70;
	size_t dword_74;
	size_t dword_78;
	size_t m_clearColor;
	size_t dword_80;
	size_t dword_84;
	size_t dword_88;
	size_t dword_8C;
};

struct WndInfo
{
	int x;
	int y;
	int w;
	int h;
};

struct SnapInfo
{
	int points[3];
};

struct CSnapInfo
{
	SnapInfo m_info[4];
};

struct StringAndColor
{
	std::string String;
	unsigned int Color;
};

struct UIWindow
{
	void * vftable;
	size_t dword_0;
	size_t dword_4;
	size_t dword_8;
	size_t dword_C;
	size_t dword_10;
	size_t dword_14;
	size_t dword_18;
	size_t dword_1C;
	size_t dword_20;
};

struct UIWindowMgr
{
	int m_chatWndX;
	int m_chatWndY;
	int m_chatWndHeight;
	int m_chatWndShow;
	int m_chatWndStatus;
	float m_miniMapZoomFactor;
	unsigned int m_miniMapArgb;
	int m_isDrawCompass;
	int m_isDragAll;
	int m_conversionMode;
	std::list<void *> m_children;
	std::list<void *> m_quitWindow;
	std::list<void *> m_nameWaitingList;
	std::map<void *, CSnapInfo> m_snapInfo;
	void *m_captureWindow;
	void *m_editWindow;
	void *m_modalWindow;
	void *m_lastHitWindow;
	void *m_loadingWnd;
	void *m_minimapZoomWnd;
	void *m_statusWnd;
	void *m_chatWnd;
	void *m_itemWnd;
	void *m_basicInfoWnd;
	void *m_equipWnd;
	void *m_optionWnd;
	void *m_shortCutWnd;
	void *m_itemDropCntWnd;
	void *m_sayDialogWnd;
	void *m_chooseWnd;
	void *m_itemIdentifyWnd;
	void *m_itemCompositionWnd;
	void *m_chooseWarpWnd;
	void *m_menu;
	void *m_comboBoxWnd;
	void *m_itemCollectionWnd;
	void *m_combinedCardItemCollectionWnd;
	void *m_itemParamChangeDisplayWnd;
	void *m_skillDescribeWnd;
	void *m_quitWnd;
	void *m_restartWnd;
	void *m_cardItemIllustWnd;
	void *m_notifyLevelUpWnd;
	void *m_notifyJobLevelUpWnd;
	void *m_itemShopWnd;
	void *m_itemPurchaseWnd;
	void *m_itemSellWnd;
	void *m_chooseSellBuyWnd;
	void *m_chatRoomMakeWnd;
	void *m_chatRoomChangeWnd;
	void *m_chatRoomWnd;
	void *m_passwordWnd;
	void *m_exchangeWnd;
	void *m_exchangeAcceptWnd;
	void *m_itemStoreWnd;
	void *m_skillListWnd;
	void *m_messengerGroupWnd;
	void *m_joinPartyAcceptWnd;
	void *m_CoupleAcceptWnd;
	void *m_BabyAcceptWnd;
	void *m_joinGuildAcceptWnd;
	void *m_allyGuildAcceptWnd;
	void *m_tipOfTheDayWnd;
	void *m_merchantItemWnd;
	void *m_merchantMirrorItemWnd;
	void *m_merchantShopMakeWnd;
	void *m_merchantItemShopWnd;
	void *m_merchantItemMyShopWnd;
	void *m_merchantItemPurchaseWnd;
	void *m_partySettingWnd;
	void *m_skillNameChangeWnd;
	void *m_npcEditDialogWnd;
	void *m_npcTextEditDialogWnd;
	void *m_detailLevelWnd;
	void *m_notifyItemObtainWnd;
	void *m_guildInfoManageWnd;
	void *m_guildMemberManageWnd;
	void *m_guildPositionManageWnd;
	void *m_guildSkillWnd;
	void *m_guildBanishedMemberWnd;
	void *m_guildNoticeWnd;
	void *m_guildTotalInfoWnd;
	void *m_whisperListWnd;
	void *m_friendOptionWnd;
	void *m_guildLeaveReasonDescWnd;
	void *m_guildBanReasonDescWnd;
	void *m_illustWnd;
	void *m_metalListWnd;
	void *m_metalProcessWnd;
	void *m_metalResultWnd;
	void *m_emotionWnd;
	void *m_emotionListWnd;
	void *m_prohibitListWnd;
	void *m_petInfoWnd;
	void *m_selectPetEggWnd;
	void *m_petTamingDeceiveWnd;
	void *m_makingArrowListWnd;
	void *m_selectCartWnd;
	void *m_spellListWnd;
	void *m_showNeedItemListWnd;
	void *m_candidateWnd;
	void *m_compositionWnd;
	void *m_keyStrokeWnd;
	int m_lastMouseX;
	int m_lastMouseY;
	int m_modalResult;
	void *m_wallpaperSurface;
	int m_w;
	int m_h;
	std::string m_wallPaperBmpName;
	std::list<StringAndColor> m_chatWndHistory;
	std::list<std::string> m_chatWndMyTalk;
	std::list<std::string> m_chatWndWhisper;
	std::string m_chatMsg;
	int m_chatWndCurHis;
	int m_chatWndWhisperCurHis;
	int m_isInvalidatedByForce;
	std::map<std::string, void *> m_whisperNameWindow;
	std::vector<std::string> m_friendNames;
	WndInfo basicInfoWndInfo;
	WndInfo itemWndInfo;
	WndInfo statusWndInfo;
	WndInfo equipWndInfo;
	WndInfo optionWndInfo;
	WndInfo shortenItemWndInfo;
	WndInfo shortenSkillWndInfo;
	WndInfo sayDialogWndInfo;
	WndInfo chooseWndInfo;
	WndInfo chatRoomWndInfo;
	WndInfo itemStoreWndInfo;
	WndInfo itemShopWndInfo;
	WndInfo itemSellWndInfo;
	WndInfo itemPurchaseWndInfo;
	WndInfo messengerGroupWndInfo;
	WndInfo shortCutWndInfo;
	WndInfo exchangeAcceptWndInfo;
	WndInfo skillListWndInfo;
	WndInfo merchantItemWndInfo;
	WndInfo merchantMirrorItemWndInfo;
	WndInfo merchantShopMakeWndInfo;
	WndInfo merchantItemShopWndInfo;
	WndInfo merchantItemMyShopWndInfo;
	WndInfo merchantItemPurchaseWndInfo;
	WndInfo itemCollectionWndInfo;
	WndInfo combinedCardItemCollectionWndInfo;
	WndInfo itemParamChangeDisplayWndInfo;
	WndInfo partySettingWndInfo;
	WndInfo detailLevelWndInfo;
	WndInfo guildWndInfo;
	WndInfo whisperWndInfo;
	WndInfo whisperListWndInfo;
	WndInfo friendOptionWndInfo;
	WndInfo itemCompositionWndInfo;
	WndInfo itemIdentifyWndInfo;
	WndInfo emotionWndInfo;
	WndInfo emotionListWndInfo;
	WndInfo petInfoWndInfo;
};
