#pragma once
#include "CFlicksyConfig.h"
#include "CMemoManager.h"
#include "CAppColorTheme.h"

class CFlicksyConfig;
class CMemoManager;
class CAppColorTheme;

struct ConfigContext
{
    CFlicksyConfig* config;
    CMemoManager* memoManager;
    CAppColorTheme* appColorTheme;
};

void ShowConfigDlg(HWND hParent, ConfigContext* lpConfigContext);
