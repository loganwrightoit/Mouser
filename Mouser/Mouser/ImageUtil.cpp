#include "stdafx.h"
#include "ImageUtil.h"

ImageUtil::ImageUtil()
{
}

ImageUtil::~ImageUtil()
{
}

HBITMAP ImageUtil::makeBitMapTransparent(HBITMAP hBitmap)
{
    HDC hdcSrc, hdcDst;
    HBITMAP hbmOld, hbmNew;
    BITMAP bm;
    COLORREF clrTP, clrBK;
    
    if ((hdcSrc = CreateCompatibleDC(NULL)) != NULL)
    {
        if ((hdcDst = CreateCompatibleDC(NULL)) != NULL)
        {
            int nRow, nCol;
            GetObject(hBitmap, sizeof(bm), &bm);
            hbmOld = (HBITMAP)SelectObject(hdcSrc, hBitmap);
            hbmNew = CreateBitmap(bm.bmWidth, bm.bmHeight, bm.bmPlanes, bm.bmBitsPixel, NULL);
            SelectObject(hdcDst, hbmNew);

            BitBlt(hdcDst, 0, 0, bm.bmWidth, bm.bmHeight, hdcSrc, 0, 0, SRCCOPY);

            clrTP = GetPixel(hdcDst, 0, 0);// Get color of first pixel at 0,0
            clrBK = GetSysColor(COLOR_MENU);// Get the current background color of the menu

            for (nRow = 0; nRow < bm.bmHeight; nRow++)// work our way through all the pixels changing their color
            for (nCol = 0; nCol < bm.bmWidth; nCol++)// when we hit our set transparency color.
            if (GetPixel(hdcDst, nCol, nRow) == clrTP)
            {
                SetPixel(hdcDst, nCol, nRow, clrBK);
            }                
        }
        DeleteDC(hdcDst);
    }
    DeleteDC(hdcSrc);

    return hbmNew;
}