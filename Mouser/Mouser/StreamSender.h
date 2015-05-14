#pragma once
#pragma comment (lib,"Gdiplus.lib")

#include <objidl.h>
#include <gdiplus.h>
#include <atlimage.h> // for CImage
#include <map>
#include <stddef.h> // CRC
#include <stdint.h> // CRC

using namespace Gdiplus;

class StreamSender
{

    public:

        struct StreamInfo
        {
            wchar_t name[256];
            int width;
            int height;
            int bpp;
        };

        StreamSender(void* peer, StreamSender::StreamInfo info);
        ~StreamSender();

        void start(HWND hWnd);
        void stop();

        int getTileSize(int x, int y);
        
    private:

        int getEncoderClsid(const WCHAR * format, CLSID * pClsid);
        void captureAsStream();
        void startCaptureThread(HWND hWnd);
        RECT getResizedDrawArea();

        void*               _peer;
        HWND                _hWnd;
        ULONG_PTR           gdiplusToken;
        GdiplusStartupInput gdiplusStartupInput;
        CLSID               clsid;
        HDC                 hSrcDC;
        HDC                 hDestDC;
        HDC                 hTileDC;
        HBITMAP             hCaptureHBmp;
        HBITMAP             hTileHBmp;
        HBITMAP             hCurrentHBmp;

        int                 _szTile;
        StreamInfo          _info;

        bool _continue;

        std::map<unsigned int, size_t> tempMap;

};