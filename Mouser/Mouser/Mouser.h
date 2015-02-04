#pragma once

#include "resource.h"

enum WindowType
{
    MouserWin,
    PeerWin,
    StreamWin,
};

HWND getRootWindow();
void updatePeerListBoxData();
void AddOutputMsg(LPWSTR msg);