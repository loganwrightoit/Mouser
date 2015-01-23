// Author: Logan Wright
// Mouser.cpp : Defines the entry point for the application.
//

#pragma once

#include "stdafx.h"
#include "Mouser.h"
#include <string>
#include <stdlib.h>
#include <atlimage.h> // IStream_ and CImage functions

using namespace std;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                        // current instance
TCHAR szTitle[MAX_LOADSTRING];          // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];    // the main window class name
HWND hOutputListBox;
HWND hCaptureScreenButton;
HWND hSendPeerDataButton;
HWND hDisconnectPeerButton;
NetworkManager *network = &NetworkManager::getInstance();
PeerHandler *peerHandler = &PeerHandler::getInstance();

HWND hMain;
HWND hStreamWindow;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    PeerWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    StreamWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MSG msg;
    HACCEL hAccelTable;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_MOUSER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MOUSER));

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wMainClass;
    WNDCLASSEX wPeerClass;
    WNDCLASSEX wStreamClass;

    // May be unnecessary
    memset(&wMainClass, 0, sizeof(WNDCLASSEX));
    memset(&wPeerClass, 0, sizeof(WNDCLASSEX));
    memset(&wStreamClass, 0, sizeof(WNDCLASSEX));

    wMainClass.cbSize = sizeof(WNDCLASSEX);
    wMainClass.style = CS_HREDRAW | CS_VREDRAW;
    wMainClass.lpfnWndProc = MainWndProc;
    wMainClass.cbClsExtra = NULL;
    wMainClass.cbWndExtra = NULL;
    wMainClass.hInstance = hInstance;
    wMainClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MOUSER));
    wMainClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wMainClass.hbrBackground = CreateSolidBrush(RGB(100, 150, 200)); // Default: (HBRUSH)(COLOR_WINDOW+1)
    wMainClass.lpszMenuName = MAKEINTRESOURCE(IDC_MOUSER);
    wMainClass.lpszClassName = szWindowClass;
    wMainClass.hIconSm = LoadIcon(wMainClass.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    // Peer window
    wPeerClass.cbSize = sizeof(WNDCLASSEX);
    wPeerClass.style = CS_HREDRAW | CS_VREDRAW;
    wPeerClass.lpfnWndProc = (WNDPROC)PeerWndProc;
    wPeerClass.cbClsExtra = NULL;
    wPeerClass.cbWndExtra = NULL;
    wPeerClass.hInstance = hInstance;
    wPeerClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MOUSER));
    wPeerClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wPeerClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wPeerClass.lpszMenuName = NULL;
    wPeerClass.lpszClassName = L"PeerClass";
    wPeerClass.hIconSm = NULL;

    // Stream window
    wStreamClass.cbSize = sizeof(WNDCLASSEX);
    wStreamClass.style = CS_HREDRAW | CS_VREDRAW;
    wStreamClass.lpfnWndProc = (WNDPROC)StreamWndProc;
    wStreamClass.cbClsExtra = NULL;
    wStreamClass.cbWndExtra = NULL;
    wStreamClass.hInstance = hInstance;
    wStreamClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MOUSER));
    wStreamClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wStreamClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wStreamClass.lpszMenuName = NULL;
    wStreamClass.lpszClassName = L"StreamClass";
    wStreamClass.hIconSm = NULL;

    return RegisterClassEx(&wMainClass) &
           RegisterClassEx(&wPeerClass) &
           RegisterClassEx(&wStreamClass);
}

HINSTANCE getHInst()
{
    return hInst;
}

void CenterWindow(HWND hWnd)
{
    int scrX = GetSystemMetrics(SM_CXSCREEN);
    int scrY = GetSystemMetrics(SM_CYSCREEN);
    RECT winRect;
    GetWindowRect(hWnd, &winRect);
    int winWidth = winRect.right - winRect.left;
    int winHeight = winRect.bottom - winRect.top;
    int newLeft = scrX / 2 - winWidth / 2;
    int newTop = scrY / 2 - winHeight / 2;
    MoveWindow(hWnd, newLeft, newTop, winWidth, winHeight, false);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hMain = CreateWindow(
       szWindowClass,       // lpClassName
       szTitle,             // lpWindowName,
       WS_OVERLAPPEDWINDOW, // dwStyle
       400,                   // x
       300,                   // y
       400,                 // nWidth
       600,                 // nHeight
       NULL,                // hWndParent
       NULL,                // hMenu
       hInstance,           // hInstance
       NULL);               // lpParam

   /*
   hStreamWindow = CreateWindow(
       L"StreamClass",      // lpClassName
       L"Streaming Window", // lpWindowName,
       WS_OVERLAPPEDWINDOW, // dwStyle
       200,                 // x
       200,                 // y
       800,                 // nWidth
       600,                 // nHeight
       NULL,                // hWndParent
       NULL,                // hMenu
       hInstance,           // hInstance
       NULL);               // lpParam
       */

   if (!hMain)// || !hStreamWindow)
   {
      return FALSE;
   }


   // DEBUG
   wchar_t buffer[256];
   swprintf(buffer, 256, L"[DEBUG]: nCmdShow: %d", nCmdShow);
   OutputDebugString(buffer);

   CenterWindow(hMain);
   ShowWindow(hMain, nCmdShow);
   UpdateWindow(hMain);

   return TRUE;
}

//
// Set window font
//
void setWindowFont(HWND hWnd)
{
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
	HFONT hFont = ::CreateFontIndirect(&ncm.lfMessageFont);
	SendMessage(hWnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
}

void AddOutputMsg(LPWSTR msg)
{
	SendMessage(hOutputListBox, LB_ADDSTRING, 0, (LPARAM)msg);
}

void DrawImage(HDC hdc, CImage image)
{
    // Image size determined by window width and height
    RECT hWndRect;
    GetWindowRect(hStreamWindow, &hWndRect);
    int hWndWidth = hWndRect.right - hWndRect.left;
    int hWndHeight = hWndRect.bottom - hWndRect.top;

    // Construct resized rect for image
    RECT imgRect;
    imgRect.left = 0;
    imgRect.right = hWndWidth;
    imgRect.top = 0;
    imgRect.bottom = hWndHeight;

    // Do corrections for aspect ratio here, and use stretch blt

    SetStretchBltMode(hdc, HALFTONE); // Smooth resize
    image.StretchBlt(hdc, imgRect);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND    - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY    - post a quit message and return
//
//
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
    HWND hMulticastButton;
   
    int width = 100;
    RECT rect;
    if (GetWindowRect(hWnd, &rect))
    {
        width = rect.right - rect.left;
    }

    switch (msg)
    {
    case WM_CREATE:

        hMain = hWnd;
        
        // Create output edit box
        hOutputListBox = CreateWindowEx(WS_EX_CLIENTEDGE,
            L"LISTBOX",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL,
            0,
            0,
            width,
            400,
            hWnd,
            (HMENU)IDC_MAIN_OUTPUT_LISTBOX,
            GetModuleHandle(NULL),
            NULL);

        // Multicast button
        hMulticastButton = CreateWindowEx(NULL,
            L"BUTTON",
            L"Multicast Discovery",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            0,  // x padding
            400, // y padding
            width / 2, // width
            30,  // height
            hWnd,
            (HMENU)IDC_MAIN_MULTICAST_DISC_BUTTON,
            GetModuleHandle(NULL),
            NULL);

        // Disconnect from peer button
        hDisconnectPeerButton = CreateWindowEx(NULL,
            L"BUTTON",
            L"Disconnect Peer",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            width / 2,  // x padding
            400, // y padding
            width / 2, // width
            30,  // height
            hWnd,
            (HMENU)IDC_MAIN_DISCONNECT_PEER_BUTTON,
            GetModuleHandle(NULL),
            NULL);

        // Send data to peer button
        hSendPeerDataButton = CreateWindowEx(NULL,
            L"BUTTON",
            L"Send Peer Data",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            0, // x padding
            430, // y padding
            width / 2, // width
            30,  // height
            hWnd,
            (HMENU)IDC_MAIN_SEND_PEER_DATA_BUTTON,
            GetModuleHandle(NULL),
            NULL);
            
        // Capture screen button
        hCaptureScreenButton = CreateWindowEx(NULL,
            L"BUTTON",
            L"Start Streaming",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            width / 2,  // x padding
            430, // y padding
            width / 2, // width
            30,  // height
            hWnd,
            (HMENU)IDC_MAIN_CAPTURE_SCREEN_BUTTON,
            GetModuleHandle(NULL),
            NULL);

        // Initialize and setup connection-based and connectionless services
        NetworkManager::getInstance().init(hMain);

        // Set window fonts
        setWindowFont(hOutputListBox);
        setWindowFont(hSendPeerDataButton);
        setWindowFont(hDisconnectPeerButton);
        setWindowFont(hMulticastButton);
        setWindowFont(hCaptureScreenButton);

        //::EnableWindow(hDisconnectPeerButton, false);
        //::EnableWindow(hSendPeerDataButton, false);
        //::EnableWindow(hCaptureScreenButton, false);

        break;
    case WM_MCST_SOCKET:
        switch (WSAGETSELECTEVENT(lParam))
        {
            case FD_READ:
                peerHandler->connectToPeer();
        }
        break;
    case WM_P2P_LISTEN_SOCKET:
        switch (WSAGETSELECTEVENT(lParam))
        {
            case FD_ACCEPT:
                peerHandler->handlePeerConnectionRequest(wParam);
        }
        break;
    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);

        // Parse the menu selections:
        switch (wmId)
        {
        case IDC_MAIN_MULTICAST_DISC_BUTTON:
        {
            std::string identifier = "CLIENT_";
            identifier.append(std::to_string(rand()));
            network->sendMulticast((char*)identifier.c_str());
        }
            break;
        case IDC_MAIN_CAPTURE_SCREEN_BUTTON:
            /*
            if (strSender == NULL)
            {
                strSender = new StreamSender(p2p_sock, GetDesktopWindow());
                thread streamThread(&StreamSender::Start, strSender);
                streamThread.detach();
                SetWindowText(hCaptureScreenButton, L"Stop Streaming");
            }
            else
            {
                strSender->Stop();
                strSender->~StreamSender();
                strSender = NULL;
                SetWindowText(hCaptureScreenButton, L"Start Streaming");
            }
            */
            break;
        case IDC_MAIN_SEND_PEER_DATA_BUTTON:
            {
                if (peerHandler->getDefaultPeer() != nullptr)
                {
                    peerHandler->getDefaultPeer()->sendStreamCursor();
                }
            }
            break;
        case IDC_MAIN_DISCONNECT_PEER_BUTTON:
            peerHandler->disconnectPeer(peerHandler->getDefaultPeer());
            break;
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            /*
            if (strSender != NULL)
            {
                strSender->~StreamSender();
            }
            */
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;
    case WM_CLOSE:
        /*
        if (p2p_sock != INVALID_SOCKET)
        {
            shutdown(p2p_sock, SD_BOTH);
            closesocket(p2p_sock);
        }
        */
        //if (p2p_lstn_sock != INVALID_SOCKET)
        //{
        //    closesocket(p2p_lstn_sock);
        //}
        //network->LeaveMulticastGroup();

        WSACleanup();
        /*
        if (strSender != NULL)
        {
            strSender->~StreamSender();
        }
        */
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// Message handler for peer window.
LRESULT CALLBACK PeerWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
    Peer* peer = (Peer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (peer == NULL)
    {
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    else
    {
        switch (msg)
        {
        case WM_COMMAND:
            wmId = LOWORD(wParam);
            wmEvent = HIWORD(wParam);

            // Parse the menu selections:
            switch (wmId)
            {

            case IDC_MAIN_CAPTURE_SCREEN_BUTTON:
                /*
                if (strSender == NULL)
                {
                strSender = new StreamSender(p2p_sock, GetDesktopWindow());
                thread streamThread(&StreamSender::Start, strSender);
                streamThread.detach();
                SetWindowText(hCaptureScreenButton, L"Stop Streaming");
                }
                else
                {
                strSender->Stop();
                strSender->~StreamSender();
                strSender = NULL;
                SetWindowText(hCaptureScreenButton, L"Start Streaming");
                }
                */
                break;
            case IDC_MAIN_SEND_PEER_DATA_BUTTON:
                peer->sendStreamCursor();
                break;
            case IDM_EXIT:
                /*
                if (strSender != NULL)
                {
                strSender->~StreamSender();
                }
                */
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, msg, wParam, lParam);
            }
            break;
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            break;
        case WM_DESTROY:
            AddOutputMsg(L"[P2P]: Peer window closed.");
            break;
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
    }
    return 0;
}

// Message handler for stream window.
LRESULT CALLBACK StreamWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (msg)
    {
        case WM_ERASEBKGND:
            AddOutputMsg(L"[DEBUG]: Stream window erasing background.");
            return FALSE;
            break;
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            break;
        case WM_DESTROY:
            AddOutputMsg(L"[P2P]: Stream closed.");
            break;
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
