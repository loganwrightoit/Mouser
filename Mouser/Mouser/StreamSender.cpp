#include "stdafx.h"
#include "StreamSender.h"

#include <atlimage.h> // for CImage

StreamSender::StreamSender(SOCKET sock, HWND hWnd)
{
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    getEncoderClsid(L"image/png", &clsid);

    RECT rect;
    GetWindowRect(hWnd, &rect);
    scrWidth = rect.right - rect.left;
    scrHeight = rect.bottom - rect.top;

    this->sock = sock;
    this->hWnd = hWnd;
    this->hSrcDC = GetDC(hWnd);
    this->hDestDC = CreateCompatibleDC(hSrcDC);
    this->hCaptureBitmap = CreateCompatibleBitmap(hSrcDC, scrWidth, scrHeight);
    SelectObject(hDestDC, hCaptureBitmap);
    
    AddOutputMsg(L"[P2P]: Stream sender created.");
}

StreamSender::~StreamSender()
{
    ReleaseDC(hWnd, hSrcDC);
    DeleteDC(hDestDC);
    DeleteObject(hCaptureBitmap);
    GdiplusShutdown(gdiplusToken);

    AddOutputMsg(L"[P2P]: Stream sender destroyed.");
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
    char * mem = new char[liSize.QuadPart];
    IStream_Reset(pStream);
    IStream_Read(pStream, mem, liSize.QuadPart);

    // Send data
    //NetworkManager::getInstance().send(sock, mem, liSize.QuadPart);

    // Release memory
    image.Destroy();
    delete[] mem;
    pStream->Release();
}

bool stopStream = false;
bool measureRate = true;

void StreamSender::start()
{
    u_long ticks = GetTickCount();
    int rate = 0;
    while (!stopStream)
    {
        captureAsStream();

        if (measureRate)
        {
            ++rate;
            u_long curTicks = GetTickCount();
            u_long elapsed = curTicks - ticks;
            if (elapsed > 1000)
            {
                wchar_t buffer[256];
                swprintf(buffer, 256, L"[P2P]: Capturing at %i frames per second.", rate);
                AddOutputMsg(buffer);
                measureRate = false;
            }
        }
    }

    stop();
}

void StreamSender::stop()
{
    stopStream = true;
}