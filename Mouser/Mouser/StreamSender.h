#pragma once
#pragma comment (lib,"Gdiplus.lib")

#include <objidl.h>
#include <gdiplus.h>

using namespace Gdiplus;

class StreamSender
{

    public:

        StreamSender(SOCKET sock, HWND hWnd);
        ~StreamSender();

        void start();
        void stop();
        
    private:

        int getEncoderClsid(const WCHAR * format, CLSID * pClsid);
        void captureAsStream();
        void receiveBitmapAsStream();
        bool captureImageToFile(LPWSTR fileName);

        SOCKET              sock;
        ULONG_PTR           gdiplusToken;
        GdiplusStartupInput gdiplusStartupInput;
        CLSID               clsid;
        HWND                hWnd;
        HDC                 hSrcDC;
        HDC                 hDestDC;
        HBITMAP             hCaptureBitmap;
        int                 scrWidth, scrHeight;

};