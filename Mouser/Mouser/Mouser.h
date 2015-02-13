#pragma once

#include "resource.h"

enum WindowType
{
    MouserWin,
    PeerWin,
    StreamWin,
};

void centerWindow(HWND hWnd, RECT newSize = { -1, -1, -1, -1 });
void CALLBACK hideChatIsTypingLabel(HWND hWnd, UINT msg, UINT timerId, DWORD dwTime);
HWND getRootWindow();
void updatePeerListBoxData();
void AddOutputMsg(LPWSTR msg);