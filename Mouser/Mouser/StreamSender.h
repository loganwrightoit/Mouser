#pragma once

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")

#include "Mouser.h"

class StreamSender
{

    public:

        StreamSender(SOCKET sock, HWND hWnd);
        ~StreamSender();

        void Start();
        void Stop();
        
    private:

        int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
        void SendBitmapAsStream();
        void ReceiveBitmapAsStream();
        bool CaptureImageToFile(LPWSTR fileName);

        SOCKET              sock;
        ULONG_PTR           gdiplusToken;
        GdiplusStartupInput gdiplusStartupInput;
        HWND                hWnd;
        HDC                 hSrcDC;
        HDC                 hDestDC;
        HBITMAP             hCaptureBitmap;
        int                 scrWidth, scrHeight;

};