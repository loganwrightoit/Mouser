#include "stdafx.h"
#include "StreamSender.h"
#include <thread>
#include "Peer.h"
#include <string>

StreamSender::StreamSender(void* peer, StreamSender::StreamInfo info)
{
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    getEncoderClsid(L"image/png", &clsid);

    _peer = peer;
    memcpy_s(&_info, sizeof(_info), &info, sizeof(_info));
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
	{
		return -1;  // Failure
	}

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
	return 1;
}

//
// Sends bitmap (or converted format) through socket as byte array.
//
void StreamSender::captureAsStream()
{
    // Update HBITMAP of screen region - CAPTUREBLT is expensive operation, so only do this once
    if (!BitBlt(hDestDC, 0, 0, _info.width, _info.height, hSrcDC, 0, 0, SRCCOPY | CAPTUREBLT))
    {
        return;
    }

    for (int x = 0; x < _info.width; x += _szTile)
    {
        for (int y = 0; y < _info.height; y += _szTile)
        {
            // Prepare stream
            IStream *pStream;
            if (CreateStreamOnHGlobal(0, TRUE, &pStream) == S_OK)
            {
                // Blit to tile HBITMAP
                if (!BitBlt(hTileDC, 0, 0, _szTile, _szTile, hDestDC, x, y, SRCCOPY))
                {
                    pStream->Release();
                    return;
                }

                // Create tile image
                CImage image;
                image.Attach(hTileHBmp);

                if (image.Save(pStream, Gdiplus::ImageFormatPNG) != S_OK)
                {
                    pStream->Release();
                    image.Destroy();
                    return;
                }

                image.Destroy();

                // Generate byte array for image
                ULARGE_INTEGER liSize;
                if (IStream_Size(pStream, &liSize) == S_OK)
                {
                    bool sendPacket = false;

                    // Generate key for image
                    unsigned int key = (x << 16) | y;

                    auto iter = tempMap.find(key);
                    if (iter == tempMap.end())
                    {
                        tempMap.insert(std::make_pair(key, (size_t)liSize.QuadPart));
                        sendPacket = true;
                    }
                    else
                    {
                        // Check if image size is different
                        if (iter->second != (size_t)liSize.QuadPart)
                        {
                            tempMap.erase(iter);
                            tempMap.insert(std::make_pair(key, (size_t)liSize.QuadPart));

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
						char * data = new char[sizeof(origin)+(size_t)liSize.QuadPart];

                        // Add origin data
						// Not zero is error condition
						if (memcpy_s(data, sizeof(origin)+(size_t)liSize.QuadPart, &origin, sizeof(origin)))
						{
							delete[] data;
							pStream->Release();
							AddOutputMsg(L"[DEBUG]: Stream memcpy_s failed, aborting.");
							return;
						}

                        // Add image data
                        if (IStream_Reset(pStream) == S_OK)
                        {
							if (IStream_Read(pStream, data + sizeof(origin), (size_t)liSize.QuadPart) == S_OK)
                            {
                                // Send data
								((Peer*)_peer)->sendPacket(new Packet(Packet::STREAM_IMAGE, data, sizeof(origin)+(size_t)liSize.QuadPart));
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
    // Set some parameters
    _szTile = getTileSize(_info.width, _info.height);

    this->hSrcDC = GetDC(hWnd);
    this->hDestDC = CreateCompatibleDC(hSrcDC);
    this->hCaptureHBmp = CreateCompatibleBitmap(hSrcDC, _info.width, _info.height);
    SelectObject(hDestDC, hCaptureHBmp);

    // Set up tile params
    hTileDC = CreateCompatibleDC(hSrcDC);
    hTileHBmp = CreateCompatibleBitmap(hDestDC, _szTile, _szTile);
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

    ((Peer*)_peer)->clearStreamSender();
}

void StreamSender::stop()
{
    stopStream = true;
    ((Peer*)_peer)->sendPacket(new Packet(Packet::STREAM_CLOSE));
}