#pragma once

#include "resource.h"

const int            DEFAULT_BUFFER_SIZE = 1464;

enum WindowType
{
    MouserWin,
    PeerWin,
    StreamWin,
};

wchar_t* getUserName();
void centerWindow(HWND hWnd, RECT newSize = { -1, -1, -1, -1 });
void CALLBACK hideChatIsTypingLabel(HWND hWnd, UINT msg, UINT timerId, DWORD dwTime);
HWND getRootWindow();
void updatePeerListBoxData();
void AddOutputMsg(LPWSTR msg);