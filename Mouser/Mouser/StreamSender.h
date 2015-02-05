#pragma once
#pragma comment (lib,"Gdiplus.lib")

#include <objidl.h>
#include <gdiplus.h>

using namespace Gdiplus;

class StreamSender
{

    public:

        StreamSender(SOCKET socket, HWND hWnd);
        ~StreamSender();

        void stream(HWND hWnd);
        void stop();
        
    private:

        int getEncoderClsid(const WCHAR * format, CLSID * pClsid);
        void captureAsStream();
        void receiveBitmapAsStream();
        bool captureImageToFile(LPWSTR fileName);

        void startCaptureThread(HWND hWnd);

        SOCKET              _socket;
        HWND                _hWnd;
        ULONG_PTR           gdiplusToken;
        GdiplusStartupInput gdiplusStartupInput;
        CLSID               clsid;
        HDC                 hSrcDC;
        HDC                 hDestDC;
        HBITMAP             hCaptureBitmap;
        int                 scrWidth, scrHeight;

};