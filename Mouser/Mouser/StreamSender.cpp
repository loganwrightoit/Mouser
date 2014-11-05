#include "StreamSender.h"

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
void StreamSender::SendBitmapAsStream()
{
    IStream* pIStream1 = NULL;
    Status stat = Ok;
    
    Gdiplus::Bitmap bmp(hCaptureBitmap, (HPALETTE)0);
    stat = bmp.Save(pIStream1, &clsid);
    STATSTG myStreamStats = { 0 };
    if (SUCCEEDED(pIStream1->Stat(&myStreamStats, 0)))
    {
        char* streamData = new char[myStreamStats.cbSize.QuadPart];
        ULONG bytesSaved = 0;
        if (SUCCEEDED(pIStream1->Read(streamData, myStreamStats.cbSize.QuadPart, &bytesSaved)))
        {
            // Send bytes to destination over socket
        }
        delete[] streamData;
    }

    pIStream1->Release();
}

void StreamSender::Start()
{
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
}

void StreamSender::Stop()
{
    // Stop stream
}