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

        StreamSender(void* peer, HWND hWnd);
        ~StreamSender();

        void stream(HWND hWnd);
        void stop();
        
    private:

        uint32_t crc(char* data, size_t len);
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
        HBITMAP             hCurrentHBmp;

        int                 szTile;
        int                 srcHeight;
        int                 srcWidth;

        std::map<unsigned int, std::pair<char*, size_t>> tileMap;


};