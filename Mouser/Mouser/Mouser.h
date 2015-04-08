#pragma once

#include "resource.h"
#include "Packet.h"
#include <vector>

const int            DEFAULT_BUFFER_SIZE = 1464;

enum WindowType
{
    MouserWin,
    PeerWin,
    StreamWin,
};

HINSTANCE getHInst();
HBRUSH getDefaultBrush();
wchar_t* getUserName();
void centerWindow(HWND hWnd, RECT newSize = { -1, -1, -1, -1 });
void CALLBACK hideChatIsTypingLabel(HWND hWnd, UINT msg, UINT timerId, DWORD dwTime);
HWND getRootWindow();
void updatePeerListBoxData();
void AddOutputMsg(LPWSTR msg);

std::vector<wchar_t*> getClasses();