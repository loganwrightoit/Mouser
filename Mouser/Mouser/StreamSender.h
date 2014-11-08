#pragma once

#include <objidl.h>
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")

class StreamSender
{

    public:

        StreamSender(SOCKET sock, HWND hWnd);
        ~StreamSender();

        void Start();
        void Stop();
        
    private:

        int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
        void CaptureAsStream();
        void ReceiveBitmapAsStream();
        bool CaptureImageToFile(LPWSTR fileName);

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