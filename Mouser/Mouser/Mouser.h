#pragma once

#include "resource.h"

enum WindowType
{
    MouserWin,
    PeerWin,
    StreamWin,
};

void updatePeerListBoxData();
HWND getWindow(WindowType type, int nCmdShow = SW_SHOWNORMAL);
void AddOutputMsg(LPWSTR msg);