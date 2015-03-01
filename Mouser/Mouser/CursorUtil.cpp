#include "stdafx.h"
#include "CursorUtil.h"
#include <thread>

CursorUtil::CursorUtil(void* peer, HWND hWnd) : _cursor({ 0, 0 }), _streaming(false)
{
    _peer = peer;
    _hWnd = hWnd;
}

CursorUtil::~CursorUtil()
{
}

RECT CursorUtil::getRect(HICON cursor)
{
    ICONINFO info;
    LONG width, height;
    if (GetIconInfo(cursor, &info))
    {
        BITMAP bmp;
        if (info.hbmColor && GetObject(info.hbmColor, sizeof(bmp), &bmp))
        {
            width = bmp.bmWidth;
            height = bmp.bmHeight;
        }
        else if (info.hbmMask && GetObject(info.hbmMask, sizeof(bmp), &bmp))
        {
            width = bmp.bmWidth;
            height = (LONG)(bmp.bmHeight / 2.0f);
        }
    }
    if (info.hbmColor)
        DeleteObject(info.hbmColor);
    if (info.hbmMask)
        DeleteObject(info.hbmMask);

    return RECT{ 0, 0, width, height };
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
            if (GetCursorPos(&p))
            {
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
                        memcpy_s(data, sizeof(_cursor), &_cursor, sizeof(_cursor));

                        // Construct and send packet
                        ((Peer*)_peer)->sendPacket(new Packet(Packet::STREAM_CURSOR, data, sizeof(_cursor)));
                    }
                }
                lastTicks = ticks;
            }            
        }
    }

    ((Peer*)_peer)->clearCursorUtil();
}

void CursorUtil::stop()
{
    _streaming = false;
}