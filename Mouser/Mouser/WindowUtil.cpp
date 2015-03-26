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
    // Skip if window parent class is this

    // Check if style is supported
    if (IsWindowVisible(hwnd) && GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_OVERLAPPEDWINDOW)
    {
        // Check if window intersects desktop
        // Non-visible windows usually exist outside desktop region
        RECT rect, desktop, result;
        GetWindowRect(hwnd, &rect);
        GetWindowRect(GetDesktopWindow(), &desktop);
        if (IntersectRect(&result, &rect, &desktop))
        {
            // Grab window title
            if (int len = GetWindowTextLength(hwnd))
            {
                wchar_t* title = new wchar_t[len];
                if (GetWindowText(hwnd, title, len))
                {
                    openWindows.push_back(hwnd);
                }
                delete[] title;
            }
        }
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