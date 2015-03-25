#pragma once

#include <vector>
#include <string>

class WindowUtil
{

    public:

        WindowUtil();
        ~WindowUtil();

        std::vector<HWND> getOpenWindows();
        std::wstring getWindowTitle(HWND hwnd);

    private:

        static BOOL CALLBACK StaticEnumWindowsProc(HWND hwnd, LPARAM lParam);
        BOOL EnumWindowsProc(HWND hwnd);

        std::vector<HWND> openWindows;

};

