#include "stdafx.h"
#include "StreamSender.h"
#include "Mouser.h"
#include <string>
#include <atlimage.h>
#include <assert.h>

StreamSender::StreamSender(SOCKET sock, HWND hWnd)
{
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    GetEncoderClsid(L"image/png", &clsid);

    RECT rect;
    scrWidth = 0;
    scrHeight = 0;
    if (GetWindowRect(hWnd, &rect))
    {
        scrWidth = rect.right - rect.left;
        scrHeight = rect.bottom - rect.top;
    }

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

int StreamSender::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
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

/*
    IMPLEMENTATION NOTES:

    To send image to client, use:
        while(  (n = fread(buf, sizeof(char *), 512, fp)) > 0)
        {
            // Ensure all bytes are sent
            sendto(sock, buf, n, &client, sizeof(client));
        }

    When putting an image buffer back together, use:
        std::ofstream("D:\\image.png", std::ios::binary).write(buf,recv_len);
*/

bool StreamSender::CaptureImageToFile(LPWSTR fileName)
{
    hCaptureBitmap = CreateCompatibleBitmap(hSrcDC, scrWidth, scrHeight);

    BitBlt(hDestDC, 0, 0, scrWidth, scrHeight, hSrcDC, 0, 0, SRCCOPY | CAPTUREBLT);
    Gdiplus::Bitmap bmp(hCaptureBitmap, (HPALETTE)0);
    CLSID pngClsid;
    GetEncoderClsid(L"image/png", &pngClsid);
    bmp.Save(fileName, &pngClsid, NULL);

    return true;
}

//
// Sends bitmap (or converted format) through socket as byte array.
//
void StreamSender::CaptureAsStream()
{
    // Load sample image
    CImage image;
    image.Attach(hCaptureBitmap);
    HRESULT hr;    
    assert(hr == S_OK);
    
    // Calculate reasonably safe buffer size
    int stride = 4 * ((image.GetWidth() + 3) / 4);
    size_t safeSize = stride * image.GetHeight() * 4 + sizeof(BITMAPINFOHEADER)+sizeof(BITMAPFILEHEADER)+256 * sizeof(RGBQUAD);
    HGLOBAL mem = GlobalAlloc(GHND, safeSize);
    assert(mem);

    // Create stream and save bitmap
    IStream* stream = 0;
    hr = CreateStreamOnHGlobal(mem, TRUE, &stream);
    assert(hr == S_OK);
    hr = image.Save(stream, Gdiplus::ImageFormatPNG);
    assert(hr == S_OK);

    // Allocate buffer for saved image
    LARGE_INTEGER seekPos = { 0 };
    ULARGE_INTEGER imageSize;
    hr = stream->Seek(seekPos, STREAM_SEEK_CUR, &imageSize);
    assert(hr == S_OK && imageSize.HighPart == 0);
    char *buffer = new char[imageSize.LowPart];

    // Fill buffer from stream
    hr = stream->Seek(seekPos, STREAM_SEEK_SET, 0);
    assert(hr == S_OK);
    hr = stream->Read(buffer, imageSize.LowPart, 0);
    assert(hr == S_OK);

    // Send bytes to destination over socket
    Send(GetPeerSocket(), buffer, (u_int)imageSize.LowPart);

    // Save to disk
    /*
    FILE* fp = 0;
    errno_t err = _wfopen_s(&fp, L"c:\\temp\\copy.bmp", L"wb");
    assert(err == 0 && fp != 0);
    fwrite(buffer, 1, imageSize.LowPart, fp);
    fclose(fp);
    */

    // Cleanup
    //stream->Release();
    delete[] buffer;

    ///////
    /*
    IStream* pIStream1 = 0;
    Status stat = Ok;

    BitBlt(hDestDC, 0, 0, scrWidth, scrHeight, hSrcDC, 0, 0, SRCCOPY | CAPTUREBLT);
    Gdiplus::Bitmap bmp(hCaptureBitmap, (HPALETTE)0);
    stat = bmp.Save(pIStream1, &clsid);

    STATSTG myStreamStats = { 0 };
    if (pIStream1->Stat(&myStreamStats, 0) == S_OK)
    {
        char* streamData = new char[myStreamStats.cbSize.QuadPart];
        ULONG bytesSaved = 0;
        if (pIStream1->Read(streamData, myStreamStats.cbSize.QuadPart, &bytesSaved) == S_OK)
        {
            // Send bytes to destination over socket
            Send(sock, streamData, bytesSaved);
        }
        delete[] streamData;
    }

    pIStream1->Release();
    */
}

void StreamSender::Start()
{
    CaptureAsStream();

    /*
    DWORD maxTicks = GetTickCount() + 1000;
    int capture = 0;
    while (GetTickCount() <= maxTicks)
    {
        wchar_t fileName[256];
        swprintf(fileName, 256, L"C:\\Users\\Logan\\Desktop\\debug\\image%i.png", capture);
        if (CaptureImageToFile(fileName)) ++capture;
    }

    wchar_t buffer[256];
    swprintf(buffer, 256, L"[DEBUG]: captured desktop at %i fps", capture);
    AddOutputMsg(buffer);
    */
}

void StreamSender::Stop()
{
    // Stop stream
}