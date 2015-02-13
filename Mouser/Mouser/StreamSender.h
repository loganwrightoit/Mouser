#pragma once
#pragma comment (lib,"Gdiplus.lib")

#include <objidl.h>
#include <gdiplus.h>
#include <atlimage.h> // for CImage
#include <map>

using namespace Gdiplus;

class StreamSender
{

    public:

        struct StreamInfo
        {
            wchar_t name[256];
            int width;
            int height;
        };

        StreamSender(void* peer, HWND hWnd);
        ~StreamSender();

        void stream(HWND hWnd);
        void stop();
        
    private:

        int getEncoderClsid(const WCHAR * format, CLSID * pClsid);
        void captureAsStream();
        bool captureImageToFile(LPWSTR fileName);
        void startCaptureThread(HWND hWnd);
        int getTileSize(int x, int y);
        bool hasChanged(std::pair<char*, size_t> mapImg, std::pair<char*, size_t> newImg);

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

        int                 szTile;
        int                 srcHeight;
        int                 srcWidth;

        std::map<unsigned int, std::pair<char*, size_t>> tileMap;

};