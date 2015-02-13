#include "stdafx.h"
#include "StreamSender.h"
#include <thread>
#include "Peer.h"
#include <string>

StreamSender::StreamSender(void* peer, HWND hWnd)
{
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    getEncoderClsid(L"image/png", &clsid);

    _peer = peer;
    _hWnd = hWnd;
}

StreamSender::~StreamSender()
{
    GdiplusShutdown(gdiplusToken);
}

int StreamSender::getEncoderClsid(const WCHAR * format, CLSID * pClsid)
{
    UINT num = 0;
    UINT size = 0;
    ImageCodecInfo* pImageCodecInfo = NULL;

    GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;  // Failure

    pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL)
    {
        return -1;  // Failure
    }        

    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j)
    {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
        {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;  // Success
        }
    }

    free(pImageCodecInfo);
}

bool StreamSender::captureImageToFile(LPWSTR fileName)
{
    hCaptureHBmp = CreateCompatibleBitmap(hSrcDC, srcWidth, srcHeight);
    BitBlt(hDestDC, 0, 0, srcWidth, srcHeight, hSrcDC, 0, 0, SRCCOPY | CAPTUREBLT);
    Gdiplus::Bitmap bmp(hCaptureHBmp, (HPALETTE)0);
    bmp.Save(fileName, &clsid, NULL);

    return true;
}

uint32_t StreamSender::getCRC(char* data, size_t len)
{
    uint32_t seed = ~(234);
    while (len--)
    {
        seed ^= *data++;
        for (int k = 0; k < 8; k++)
        {
            seed = seed & 1 ? (seed >> 1) ^ 0x82f63b78 : seed >> 1;
        }
    }
    return ~seed;
}

//
// Sends bitmap (or converted format) through socket as byte array.
//
void StreamSender::captureAsStream()
{
    // Update HBITMAP of screen region - CAPTUREBLT is expensive operation, so only do this once
    BitBlt(hDestDC, 0, 0, srcWidth, srcHeight, hSrcDC, 0, 0, SRCCOPY | CAPTUREBLT);

    for (int x = 0; x < srcWidth; x += szTile)
    {
        for (int y = 0; y < srcHeight; y += szTile)
        {
            // Prepare stream
            IStream *pStream;
            if (CreateStreamOnHGlobal(0, TRUE, &pStream) == S_OK)
            {
                // Blit to tile HBITMAP
                BitBlt(hTileDC, 0, 0, szTile, szTile, hDestDC, x, y, SRCCOPY);

                // Create tile image
                CImage image;
                image.Attach(hTileHBmp);
                image.Save(pStream, Gdiplus::ImageFormatPNG);
                image.Destroy();

                // Generate byte array for image
                ULARGE_INTEGER liSize;
                if (IStream_Size(pStream, &liSize) == S_OK)
                {
                    bool sendPacket = false;

                    // Generate key for image
                    unsigned int key = (x << 16) | y;

                    // Generate temporary byte strings for image processing
                    char* bytes = new char[liSize.QuadPart];
                    memcpy(bytes, pStream, liSize.QuadPart);

                    // Check if image region is different than cache
                    uint32_t crc = getCRC(bytes, liSize.QuadPart);
                    auto iter = tempMap.find(key);
                    if (iter == tempMap.end())
                    {
                        tempMap.insert(std::make_pair(key, crc));
                        sendPacket = true;
                    }
                    else
                    {
                        if (iter->second != crc)
                        {
                            tempMap.erase(iter);
                            tempMap.insert(std::make_pair(key, crc));

                            sendPacket = true;
                        }
                    }
                    
                    if (sendPacket)
                    {
                        // Construct origin point
                        POINT origin;
                        origin.x = x;
                        origin.y = y;

                        // Copy capture bytes to array
                        char * data = new char[sizeof(origin)+liSize.QuadPart];

                        // Add origin data
                        std::memcpy(data, &origin, sizeof(origin));

                        // Add image data
                        if (IStream_Reset(pStream) == S_OK)
                        {
                            if (IStream_Read(pStream, data + sizeof(origin), liSize.QuadPart) == S_OK)
                            {
                                // Send data
                                ((Peer*)_peer)->sendPacket(new Packet(Packet::STREAM_IMAGE, data, sizeof(origin)+liSize.QuadPart));
                            }
                        }                        
                    }
                }            
            }
            pStream->Release();
        }
    }
}

bool stopStream = false;

void StreamSender::stream(HWND hWnd)
{
    // Set common variables
    RECT rect;
    GetWindowRect(hWnd, &rect);
    srcWidth = rect.right - rect.left;
    srcHeight = rect.bottom - rect.top;
    szTile = getTileSize(srcWidth, srcHeight);

    // Send stream info (window size, title)
    StreamInfo info;
    info.width = srcWidth;
    info.height = srcHeight;
    info.bpp = 32; // Hardcoded, need to figure out how to get real value
    if (!GetWindowText(hWnd, info.name, 256))
    {
        if (hWnd == GetDesktopWindow())
        {
            wcscpy(info.name, L"Desktop");
        }
        else
        {
            wcscpy(info.name, L"Shared Window");
        }
    }

    // Send StreamInfo packet
    char * data = new char[sizeof(info)];
    std::memcpy(data, &info, sizeof(info));
    ((Peer*)_peer)->sendPacket(new Packet(Packet::STREAM_INFO, data, sizeof(info)));

    // Start stream
    std::thread t(&StreamSender::startCaptureThread, this, hWnd);
    t.detach();
}

int StreamSender::getTileSize(int x, int y)
{
    return (y == 0) ? x * 8 : getTileSize(y, x % y);
}

//
//  Pass window that should be captured.
//
void StreamSender::startCaptureThread(HWND hWnd)
{
    // Set up common params
    RECT rect;
    GetWindowRect(hWnd, &rect);
    srcWidth = rect.right - rect.left;
    srcHeight = rect.bottom - rect.top;
    szTile = getTileSize(srcWidth, srcHeight);

    this->hSrcDC = GetDC(hWnd);
    this->hDestDC = CreateCompatibleDC(hSrcDC);
    this->hCaptureHBmp = CreateCompatibleBitmap(hSrcDC, srcWidth, srcHeight);
    SelectObject(hDestDC, hCaptureHBmp);

    // Set up tile params
    hTileDC = CreateCompatibleDC(hSrcDC);
    hTileHBmp = CreateCompatibleBitmap(hDestDC, szTile, szTile);
    SelectObject(hTileDC, hTileHBmp);

    while (!stopStream)
    {
        if ((((Peer*)_peer)->getQueueSize() < 4))
        {
            captureAsStream();
        }
    }

    // Release memory
    ReleaseDC(_hWnd, hSrcDC);
    ReleaseDC(_hWnd, hDestDC);
    DeleteDC(hDestDC);
    DeleteObject(hCaptureHBmp);
    ReleaseDC(_hWnd, hTileDC);
    DeleteDC(hTileDC);
    DeleteObject(hTileHBmp);
}

void StreamSender::stop()
{
    stopStream = true;
}