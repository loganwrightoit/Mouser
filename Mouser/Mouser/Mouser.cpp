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
HWND hOutputListBox;
SOCKET mcst_lstn_sock = INVALID_SOCKET;
SOCKET bcst_lstn_sock = INVALID_SOCKET;
SOCKET p2p_lstn_sock = INVALID_SOCKET;
SOCKET p2p_sock = INVALID_SOCKET;

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

        if (connect(p2p_sock, (LPSOCKADDR)(&addr), sizeof(addr)) == SOCKET_ERROR)
        {
            wchar_t buffer[256];
            swprintf(buffer, 256, L"[P2P]: connect() failed with error: %i", WSAGetLastError());
            AddOutputMsg(buffer);
            return;
        }
        if (WSAAsyncSelect(p2p_sock, hMain, WM_P2P_SOCKET, (FD_CLOSE | FD_READ | FD_WRITE)) == SOCKET_ERROR)
        {
            wchar_t buffer[256];
            swprintf(buffer, 256, L"[P2P]: WSAAsyncSelect() failed with error: %i", WSAGetLastError());
            AddOutputMsg(buffer);
            return;
        }

        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: Connected to peer at %hs", inet_ntoa(addr.sin_addr));
        AddOutputMsg(buffer);
    }
}

//
// Sets up UDP broadcast listener.
//
void SetupBroadcastListener()
{
    bcst_lstn_sock = GetBroadcastSocket();
    if (bcst_lstn_sock != INVALID_SOCKET)
    {
        if (WSAAsyncSelect(bcst_lstn_sock, hMain, WM_BCST_SOCKET, FD_READ))
        {
            wchar_t buffer[256];
            swprintf(buffer, 256, L"[Broadcast]: WSAAsyncSelect() failed with error: %i", WSAGetLastError());
            AddOutputMsg(buffer);
        }
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
        if (WSAAsyncSelect(mcst_lstn_sock, hMain, WM_MCST_SOCKET, FD_READ))
        {
            wchar_t buffer[256];
            swprintf(buffer, 256, L"[Multicast]: WSAAsyncSelect() failed with error: %i", WSAGetLastError());
            AddOutputMsg(buffer);
        }
    }
}

//
// Sets up TCP connection listener.
//
void SetupConnectionListener()
{
    if ((p2p_lstn_sock = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: socket() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
    }
    if (WSAAsyncSelect(p2p_lstn_sock, hMain, WM_P2P_LISTEN_SOCKET, (FD_ACCEPT | FD_CLOSE)) != 0)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: WSAAsyncSelect() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(GetPrimaryClientPort());

    if (::bind(p2p_lstn_sock, (struct sockaddr*) &addr, sizeof(addr)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: bind() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
    }
    if (listen(p2p_lstn_sock, 5))
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: listen() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
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
	HWND hBroadcastButton,
         hMulticastButton,
         hIpBox = NULL,
         hDirectConnectButton,
         hPrimaryConnection,
         hSendPeerDataButton,
         hDisconnectPeerButton;

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

        // Send data to peer button
        hSendPeerDataButton = CreateWindowEx(NULL,
            L"BUTTON",
            L"Send Peer Data",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            0,  // x padding
            400, // y padding
            width / 2, // width
            30,  // height
            hWnd,
            (HMENU)IDC_MAIN_SEND_PEER_DATA_BUTTON,
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

        // Broadcast button
        hBroadcastButton = CreateWindowEx(NULL,
            L"BUTTON",
            L"Broadcast Discovery",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            0,  // x padding
            430, // y padding
            width / 2, // width
            30,  // height
            hWnd,
            (HMENU)IDC_MAIN_BROADCAST_DISC_BUTTON,
            GetModuleHandle(NULL),
            NULL);

        // Multicast button
        hMulticastButton = CreateWindowEx(NULL,
            L"BUTTON",
            L"Multicast Discovery",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            width / 2,  // x padding
            430, // y padding
            width / 2, // width
            30,  // height
            hWnd,
            (HMENU)IDC_MAIN_MULTICAST_DISC_BUTTON,
            GetModuleHandle(NULL),
            NULL);

        // Manual IP connect button
        hDirectConnectButton = CreateWindowEx(NULL,
            L"BUTTON",
            L"Direct Connect",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            0,  // x padding
            460, // y padding
            width / 2, // width
            30,  // height
            hWnd,
            (HMENU)IDC_MAIN_DIRECT_CONNECT_BUTTON,
            GetModuleHandle(NULL),
            NULL);

        // IP edit box
        hIpBox = CreateWindowEx(WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | ES_WANTRETURN | WS_VISIBLE | ES_CENTER,
            width / 2,  // x padding
            460, // y padding
            width / 2, // width
            30,  // height
            hWnd,
            (HMENU)IDC_MAIN_IP_TEXTBOX,
            GetModuleHandle(NULL),
            NULL);

        /*
        // create the window and use the result as the handle
        hPrimaryConnection = CreateWindowEx(NULL,
            L"primaryConnection", // name of the window class
            L"Connected Client", // title of the window
            WS_OVERLAPPEDWINDOW, // window style
            300, // x-position of the window
            300, // y-position of the window
            500, // width of the window
            400, // height of the window
            HWND_DESKTOP, // we have no parent window, NULL
            NULL, // we aren't using menus, NULL
            hInst, // application handle
            NULL); // used with multiple windows, NULL
        */

        // Initialize and setup connection-based and connectionless services
        InitWinsock();
        SetupBroadcastListener();
        SetupMulticastListener();
        SetupConnectionListener();
        
        // Set text length limit for IP box
        SendMessage(hIpBox, EM_LIMITTEXT, 15, NULL);

        setWindowFont(hOutputListBox);
        setWindowFont(hSendPeerDataButton);
        setWindowFont(hDisconnectPeerButton);
        setWindowFont(hBroadcastButton);
        setWindowFont(hMulticastButton);
        setWindowFont(hIpBox);
        setWindowFont(hDirectConnectButton);

        break;
    case WM_BCST_SOCKET:
        switch (WSAGETSELECTEVENT(lParam))
        {
        case FD_READ:
            if (p2p_sock == INVALID_SOCKET)
            {
                sockaddr_in addr = GetBroadcastSenderInfo(bcst_lstn_sock);
                thread clientThread(ConnectToPeerThread, addr);
                clientThread.detach();
            }
            break;
        }
        break;
    case WM_MCST_SOCKET:
        switch (WSAGETSELECTEVENT(lParam))
        {
        case FD_READ:
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
        switch (WSAGETSELECTEVENT(lParam))
        {
        case FD_CONNECT:
            //AddOutputMsg(L"[P2P]: FD_CONNECT event raised.");
            break;
        case FD_ACCEPT:
            //AddOutputMsg(L"[P2P]: FD_ACCEPT event raised.");
            if (p2p_sock == INVALID_SOCKET)
            {
                if ((p2p_sock = accept(wParam, NULL, NULL)) == INVALID_SOCKET)
                {
                    wchar_t buffer[256];
                    swprintf(buffer, 256, L"[P2P]: accept() failed with error: %i", WSAGetLastError());
                    AddOutputMsg(buffer);
                    break;
                }
                if (WSAAsyncSelect(p2p_sock, hWnd, WM_P2P_SOCKET, (FD_READ | FD_WRITE | FD_CLOSE)) == SOCKET_ERROR)
                {
                    wchar_t buffer[256];
                    swprintf(buffer, 256, L"[P2P]: WSAAsyncSelect() failed with error: %i", WSAGetLastError());
                    AddOutputMsg(buffer);
                }
                sockaddr_in addr;
                int size = sizeof(addr);
                getpeername(p2p_sock, (struct sockaddr *)&addr, &size);
                wchar_t buffer[256];
                swprintf(buffer, 256, L"[P2P]: Connected to peer at %hs", inet_ntoa(addr.sin_addr));
                AddOutputMsg(buffer);
            }
            break;
        }
    case WM_P2P_SOCKET:
        switch (WSAGETSELECTEVENT(lParam))
        {
        case FD_READ:
            AddOutputMsg(L"[P2P]: FD_READ event raised.");
            break;
        case FD_WRITE:
            //AddOutputMsg(L"[P2P]: FD_WRITE event raised.");
            break;
        case FD_CLOSE:
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
                AddOutputMsg(L"[P2P]: Peer closed connection.");
            }
            p2p_sock = INVALID_SOCKET;
            break;
        }
        break;
    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
		case IDC_MAIN_BROADCAST_DISC_BUTTON:
            SendBroadcast(bcst_lstn_sock);
			break;
		case IDC_MAIN_MULTICAST_DISC_BUTTON:
			SendMulticast(mcst_lstn_sock);
			break;
		case IDC_MAIN_DIRECT_CONNECT_BUTTON:
			TCHAR buff[16];
			GetWindowText(hIpBox, buff, sizeof(buff));
			if (_tcslen(buff) > 0) {
				AddOutputMsg(L"[P2P] Connecting to host");
			}
			else {
				AddOutputMsg(L"[P2P] Please input an IP address");
			}
			break;
        case IDC_MAIN_SEND_PEER_DATA_BUTTON:
            if (p2p_sock != INVALID_SOCKET)
            {
                Send(p2p_sock, "This is a message!");
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
                    AddOutputMsg(L"[P2P]: Peer closed connection.");
                }
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
        closesocket(bcst_lstn_sock);
        CloseMulticast(mcst_lstn_sock);
        WSACleanup();
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
