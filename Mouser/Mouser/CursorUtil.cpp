#include "stdafx.h"
#include "CursorUtil.h"
#include <thread>

CursorUtil::CursorUtil(void* peer, HWND hWnd) : _streaming(false)
{
    _peer = peer;
    _hWnd = hWnd;
}

CursorUtil::~CursorUtil()
{
}

POINT CursorUtil::getCursor()
{
    return _cursor;
}

void CursorUtil::stream(int sendRate)
{
    _streaming = true;

    // Start cursor stream thread
    std::thread t(&CursorUtil::streamThread, this, sendRate);
    t.detach();
}

void CursorUtil::streamThread(int sendRate)
{
    UINT lastTicks = GetTickCount();
    float timeFactor = 1000 / 60.0f;

    while (_streaming)
    {
        // Send cursor updates according to sendRate
        UINT ticks = GetTickCount();
        if (((lastTicks - ticks) / timeFactor) >= sendRate)
        { 
            POINT p;
            GetCursorPos(&p);
            HWND hWnd = GetDesktopWindow();
            if (ScreenToClient(hWnd, &p))
            {
                // Only send update if cursor location changed
                if (_cursor.x != p.x || _cursor.y != p.y)
                {
                    _cursor.x = p.x;
                    _cursor.y = p.y;

                    // Convert struct to char array
                    char * data = new char[sizeof(_cursor)];
                    std::memcpy(data, &_cursor, sizeof(_cursor));

                    // Construct and send packet
                    SendMessage(((Peer*)_peer)->getRoot(), WM_EVENT_SEND_PACKET, (WPARAM) new Packet(Packet::STREAM_CURSOR, data, sizeof(_cursor)), NULL);
                }
            }

            lastTicks = ticks;
        }
    }
}

void CursorUtil::stop()
{
    _streaming = false;
}