#pragma once

#include "resource.h"

enum WindowType
{
    MouserWin,
    PeerWin,
    StreamWin,
};

HWND getPeerList();
HWND getWindow(WindowType type, int nCmdShow = SW_SHOWNORMAL);
void AddOutputMsg(LPWSTR msg);