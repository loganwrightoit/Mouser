// Author: Logan Wright
// Mouser.cpp : Defines the entry point for the application.
//

#pragma once

#include "stdafx.h"
#include "Mouser.h"
#include <string>
#include <thread>

using namespace std;

#define MAX_LOADSTRING 100
#define DEFAULT_PORT 41000

// Global Variables:
HINSTANCE hInst;                        // current instance
TCHAR szTitle[MAX_LOADSTRING];          // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];    // the main window class name
HWND hOutputListBox, hCaptureScreenButton;
SOCKET mcst_lstn_sock = INVALID_SOCKET;
SOCKET p2p_lstn_sock = INVALID_SOCKET;
SOCKET p2p_sock = INVALID_SOCKET;
StreamSender *strSender = NULL;

SOCKET GetPeerSocket()
{
    return p2p_sock;
}

HWND hMain;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
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
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MOUSER));
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = CreateSolidBrush(RGB(100, 150, 200)); // Default: (HBRUSH)(COLOR_WINDOW+1)
    wcex.lpszMenuName  = MAKEINTRESOURCE(IDC_MOUSER);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
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
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(
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

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

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

void ListenToPeerThread()
{
    while (p2p_sock != INVALID_SOCKET)
    {
        fd_set mySet;
        FD_ZERO(&mySet);
        FD_SET(p2p_sock, &mySet);
        timeval zero = { 0, 0 };
        int sel = select(0, &mySet, NULL, NULL, &zero);
        if (FD_ISSET(p2p_sock, &mySet))
        {
            u_int length = GetReceiveLength(p2p_sock);
            if (length == SOCKET_ERROR)
            {
                // Shutdown
                wchar_t buffer1[256];
                swprintf(buffer1, 256, L"[P2P]: Peer socket failed with error: %i", WSAGetLastError());
                AddOutputMsg(buffer1);
                closesocket(p2p_sock);
                return;
            }
            else if (length == 0)
            {
                AddOutputMsg(L"[P2P]: Peer shutdown socket.");
                closesocket(p2p_sock);
                return;
            }
            else
            {
                // Receive remainder of message
                char * buffer = new char[length];
                if (Receive(p2p_sock, buffer, length))
                {
                    // Process data here
                    wchar_t buffer1[256];
                    swprintf(buffer1, 256, L"[P2P]: Received %i bytes.", length);
                    AddOutputMsg(buffer1);
                }
                delete[] buffer;
            }
        }
    }
}

void ConnectToPeerThread(sockaddr_in inAddr)
{
    if (p2p_sock == INVALID_SOCKET)
    {
        p2p_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (p2p_sock == INVALID_SOCKET)
        {
            wchar_t buffer[256];
            swprintf(buffer, 256, L"[P2P]: socket() failed with error: %i", WSAGetLastError());
            AddOutputMsg(buffer);
            return;
        }

        // Set up our socket address structure
        SOCKADDR_IN addr;
        addr.sin_port = htons(GetPrimaryClientPort());
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inAddr.sin_addr.S_un.S_addr;

        if (connect(p2p_sock, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
        {
            wchar_t buffer[256];
            swprintf(buffer, 256, L"[P2P]: connect() failed with error: %i", WSAGetLastError());
            AddOutputMsg(buffer);
            return;
        }

        // Create new thread for incoming P2P data
        thread peerThread(ListenToPeerThread);
        peerThread.detach();

        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: Connected to peer at %hs", inet_ntoa(addr.sin_addr));
        AddOutputMsg(buffer);

        ::EnableWindow(hCaptureScreenButton, true);
    }
}

//
// Sets up UDP multicast listener.
//
void SetupMulticastListener()
{
    mcst_lstn_sock = GetMulticastSocket();
    if (mcst_lstn_sock != INVALID_SOCKET)
    {
        if (WSAAsyncSelect(mcst_lstn_sock, hMain, WM_MCST_SOCKET, FD_READ) == SOCKET_ERROR)
        {
            wchar_t buffer[256];
            swprintf(buffer, 256, L"[Multicast]: WSAAsyncSelect() failed with error: %i", WSAGetLastError());
            AddOutputMsg(buffer);
            closesocket(mcst_lstn_sock);
        }
    }
}

//
// Sets up TCP connection listener.
//
void SetupConnectionListener()
{
    p2p_lstn_sock = socket(PF_INET, SOCK_STREAM, 0);

    if (p2p_lstn_sock == INVALID_SOCKET)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: socket() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        return;
    }
    if (WSAAsyncSelect(p2p_lstn_sock, hMain, WM_P2P_LISTEN_SOCKET, (FD_CONNECT | FD_ACCEPT | FD_CLOSE)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: WSAAsyncSelect() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(p2p_lstn_sock);
        return;
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(GetPrimaryClientPort());

    if (::bind(p2p_lstn_sock, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: bind() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(p2p_lstn_sock);
        return;
    }
    if (listen(p2p_lstn_sock, 5) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: listen() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(p2p_lstn_sock);
        return;
    }
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
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
    HWND hMulticastButton;
    HWND hSendPeerDataButton;
    HWND hDisconnectPeerButton;
    //HWND hCaptureScreenButton;
   
    int width = 100;
    RECT rect;
    if (GetWindowRect(hWnd, &rect))
    {
        width = rect.right - rect.left;
    }

    switch (message)
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
        InitWinsock();
        SetupMulticastListener();
        SetupConnectionListener();
        setWindowFont(hOutputListBox);
        setWindowFont(hSendPeerDataButton);
        setWindowFont(hDisconnectPeerButton);
        setWindowFont(hMulticastButton);
        setWindowFont(hCaptureScreenButton);

        break;
    case WM_MCST_SOCKET:
        switch (WSAGETSELECTEVENT(lParam))
        {
        case FD_READ:
            //AddOutputMsg(L"[Multicast]: FD_READ event raised.");
            if (p2p_sock == INVALID_SOCKET)
            {
                sockaddr_in addr = GetMulticastSenderInfo(mcst_lstn_sock);
                thread clientThread(ConnectToPeerThread, addr);
                clientThread.detach();
            }
            break;
        }
        break;
    case WM_P2P_LISTEN_SOCKET:

        //
        // This socket listens for incoming peer connection requests
        // on the TCP connection protocol.  Once a request is received,
        // a new socket will be created between both clients.
        //

        switch (WSAGETSELECTEVENT(lParam))
        {
        case FD_CONNECT:
            //AddOutputMsg(L"[P2P]: FD_CONNECT event raised.");
            break;
        case FD_ACCEPT: // Client that initiated multicast receives this
            //AddOutputMsg(L"[P2P]: FD_ACCEPT event raised.");
            if (p2p_sock == INVALID_SOCKET)
            {
                p2p_sock = accept(wParam, NULL, NULL);
                if (p2p_sock == INVALID_SOCKET)
                {
                    wchar_t buffer[256];
                    swprintf(buffer, 256, L"[P2P]: accept() failed with error: %i", WSAGetLastError());
                    AddOutputMsg(buffer);
                    closesocket(p2p_sock);
                    break;
                }

                // Create new thread for incoming P2P data
                thread peerThread(ListenToPeerThread);
                peerThread.detach();

                sockaddr_in temp_addr;
                int size = sizeof(temp_addr);
                getpeername(p2p_sock, (LPSOCKADDR)&temp_addr, &size);
                wchar_t buffer[256];
                swprintf(buffer, 256, L"[P2P]: Connected to peer at %hs", inet_ntoa(temp_addr.sin_addr));
                AddOutputMsg(buffer);
                ::EnableWindow(hCaptureScreenButton, true);
            }
        }

        break;
    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
		case IDC_MAIN_MULTICAST_DISC_BUTTON:
			SendMulticast(mcst_lstn_sock);
			break;
        case IDC_MAIN_CAPTURE_SCREEN_BUTTON:
            // if (p2p_sock != INVALID_SOCKET)
            // {
            if (strSender == NULL)
            {
                //AddOutputMsg(L"DEBUG: Started streaming desktop.");
                //strSender = new StreamSender(p2p_sock, GetDesktopWindow());
                //strSender->Start();
                SetWindowText(hCaptureScreenButton, L"Stop Streaming");

                char * test = new char[2560000];
                Send(p2p_sock, test, 2560000);
                delete[] test;
            }
            else
            {
                AddOutputMsg(L"[P2P]: Stopped streaming desktop.");
                strSender->Stop();
                SetWindowText(hCaptureScreenButton, L"Start Streaming");
            }
            //}
            break;
        case IDC_MAIN_SEND_PEER_DATA_BUTTON:
            if (p2p_sock != INVALID_SOCKET)
            {
                char * test = new char[256000];
                Send(p2p_sock, test, 256000);
                delete[] test;
            }
            else
            {
                AddOutputMsg(L"[P2P]: Must connect to a peer before sending data.");
            }
            break;
        case IDC_MAIN_DISCONNECT_PEER_BUTTON:
            if (p2p_sock != INVALID_SOCKET)
            {
                if (shutdown(p2p_sock, SD_SEND) == SOCKET_ERROR)
                {
                    wchar_t buffer[256];
                    swprintf(buffer, 256, L"[P2P]: shutdown() failed with error: %i", WSAGetLastError());
                    AddOutputMsg(buffer);
                }
                if (closesocket(p2p_sock) == SOCKET_ERROR)
                {
                    wchar_t buffer[256];
                    swprintf(buffer, 256, L"[P2P]: closesocket() failed with error: %i", WSAGetLastError());
                    AddOutputMsg(buffer);
                }
                else
                {
                    AddOutputMsg(L"[P2P]: Connection closed.");
                }

                p2p_sock = INVALID_SOCKET;

                // Disable button and change text
                ::EnableWindow(hCaptureScreenButton, false);
                SetWindowText(hCaptureScreenButton, L"Start Streaming");

            }
            else
            {
                AddOutputMsg(L"[P2P]: Must connect to a peer before disconnecting.");
            }
            p2p_sock = INVALID_SOCKET;
            break;
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            if (strSender != NULL)
            {
                strSender->~StreamSender();
            }
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;
    case WM_CLOSE:
        CloseMulticast(mcst_lstn_sock);
        WSACleanup();
        if (strSender != NULL)
        {
            MessageBox(hWnd, L"Cleaning up stream sender.", L"INFO", 0);
            strSender->~StreamSender();
        }
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
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
