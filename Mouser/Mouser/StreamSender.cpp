#include "stdafx.h"
#include "StreamSender.h"
#include <thread>
#include "Peer.h"
#include <string>

#include <atlimage.h> // for CImage

StreamSender::StreamSender(void* peer, HWND hWnd)
{
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    getEncoderClsid(L"image/png", &clsid);

    _peer = peer;
    _hWnd = hWnd;
}

StreamSender::~StreamSender()
{
    ReleaseDC(_hWnd, hSrcDC);
    DeleteDC(hDestDC);
    DeleteObject(hCaptureBitmap);
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
    hCaptureBitmap = CreateCompatibleBitmap(hSrcDC, scrWidth, scrHeight);

    BitBlt(hDestDC, 0, 0, scrWidth, scrHeight, hSrcDC, 0, 0, SRCCOPY | CAPTUREBLT);
    Gdiplus::Bitmap bmp(hCaptureBitmap, (HPALETTE)0);
    bmp.Save(fileName, &clsid, NULL);

    return true;
}

//
// Sends bitmap (or converted format) through socket as byte array.
//
void StreamSender::captureAsStream()
{
    // Prepare stream
    IStream *pStream;
    HRESULT result = CreateStreamOnHGlobal(0, TRUE, &pStream);

    // Save window capture to stream
    BitBlt(hDestDC, 0, 0, scrWidth, scrHeight, hSrcDC, 0, 0, SRCCOPY | CAPTUREBLT);
    CImage image;
    image.Attach(hCaptureBitmap);
    image.Save(pStream, Gdiplus::ImageFormatPNG);

    // Copy capture bytes to array
    ULARGE_INTEGER liSize;
    IStream_Size(pStream, &liSize);
    char * data = new char[liSize.QuadPart];
    IStream_Reset(pStream);
    IStream_Read(pStream, data, liSize.QuadPart);

    // Send data
    ((Peer*)_peer)->sendPacket(new Packet(Packet::STREAM_IMAGE, data, liSize.QuadPart));

    // Release memory
    image.Destroy();
    pStream->Release();
}

bool stopStream = false;

void StreamSender::stream(HWND hWnd)
{
    std::thread t(&StreamSender::startCaptureThread, this, hWnd);
    t.detach();
}

//
//  Pass window that should be captured.
//
void StreamSender::startCaptureThread(HWND hWnd)
{
    RECT rect;
    GetWindowRect(hWnd, &rect);
    scrWidth = rect.right - rect.left;
    scrHeight = rect.bottom - rect.top;

    this->hSrcDC = GetDC(hWnd);
    this->hDestDC = CreateCompatibleDC(hSrcDC);
    this->hCaptureBitmap = CreateCompatibleBitmap(hSrcDC, scrWidth, scrHeight);
    SelectObject(hDestDC, hCaptureBitmap);

    u_long ticks = GetTickCount();
    int rate = 0;
    while (!stopStream)
    {
        if ((((Peer*)_peer)->getQueueSize() < 4))
        {
            std::wstring str(L"[DEBUG]: queueSize(): ");
            str.append(std::to_wstring(((Peer*)_peer)->getQueueSize()));
            str.append(L"\n");
            AddOutputMsg((LPWSTR)str.c_str());
            captureAsStream();
        }
    }
}

void StreamSender::stop()
{
    stopStream = true;
}