#include "stdafx.h"
#include "WindowUtil.h"

WindowUtil::WindowUtil()
{
}

WindowUtil::~WindowUtil()
{
}

std::vector<HWND> WindowUtil::getOpenWindows()
{
    openWindows.clear();
    EnumWindows(StaticEnumWindowsProc, reinterpret_cast<LPARAM>(this));
    return openWindows;
}

BOOL CALLBACK WindowUtil::StaticEnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    WindowUtil *pThis = reinterpret_cast<WindowUtil*>(lParam);
    return pThis->EnumWindowsProc(hwnd);
}

BOOL WindowUtil::EnumWindowsProc(HWND hwnd)
{
    if (IsWindowVisible(hwnd))
    {
        WINDOWPLACEMENT wnd;
        GetWindowPlacement(hwnd, &wnd);

        //if (wnd.ptMinPosition.x < -1) // Seems to omit this program's windows, and abnormal window types
        //{
            // Grab window title
            int len = 0;
            if (len = GetWindowTextLength(hwnd))
            {
                wchar_t* title = new wchar_t[len];
                if (GetWindowText(hwnd, title, len))
                {
                    OutputDebugString(std::to_wstring(wnd.rcNormalPosition.bottom).c_str());
                    OutputDebugString(L", ");
                    OutputDebugString(std::to_wstring(wnd.ptMaxPosition.x).c_str());
                    OutputDebugString(L", ");
                    OutputDebugString(std::to_wstring(wnd.ptMinPosition.x).c_str());
                    OutputDebugString(L": ");
                    OutputDebugString(title);
                    OutputDebugString(L"\n");

                    openWindows.push_back(hwnd);
                }
                delete[] title;
            }
        //}
    }

    return TRUE;
}

std::wstring WindowUtil::getWindowTitle(HWND hwnd)
{
    // Grab window title
    int len = GetWindowTextLength(hwnd) + 1;
    wchar_t* title = new wchar_t[len];
    GetWindowText(hwnd, title, len);

    return std::wstring(title);
}