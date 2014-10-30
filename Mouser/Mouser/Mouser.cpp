// Author: Logan Wright
// Mouser.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Mouser.h"
#include <string>

using namespace std;

// Default connection port
#define CONN_PORT 41920
#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                        // current instance
TCHAR szTitle[MAX_LOADSTRING];          // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];    // the main window class name
HWND hOutputListBox;
SOCKET mcst_lstn_sock = INVALID_SOCKET, bcst_lstn_sock = INVALID_SOCKET, conn_socket = INVALID_SOCKET;

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
       400,                 // x
       50,                  // y
       480,                 // nWidth
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
         hMulticastTTLBox,
         hIpBox = NULL,
         hDirectConnectButton,
         hPrimaryConnection;

	int ttl = 1;
    int width = 100;
    int height = 100;
    RECT rect;
    if (GetWindowRect(hWnd, &rect))
    {
        width = rect.right - rect.left;
        //height = rect.bottom - rect.top;
    }

    switch (message)
    {
    case WM_CREATE:

		// Create output edit box
		hOutputListBox = CreateWindowEx(WS_EX_CLIENTEDGE,
            L"LISTBOX",
            L"",
            WS_CHILD | WS_VISIBLE,
            20,
            20,
            425,
            height,
            hWnd,
            (HMENU)IDC_MAIN_OUTPUT_LISTBOX,
            GetModuleHandle(NULL),
            NULL);

		// Broadcast button
		hBroadcastButton = CreateWindowEx(NULL,
			L"BUTTON",
			L"Broadcast Discovery",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			50,  // x padding
			220, // y padding
			150, // width
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
			50,  // x padding
			270, // y padding
			150, // width
			30,  // height
			hWnd,
			(HMENU)IDC_MAIN_MULTICAST_DISC_BUTTON,
			GetModuleHandle(NULL),
			NULL);

		// Multicast TTL box
		hMulticastTTLBox = CreateWindowEx(WS_EX_CLIENTEDGE,
			L"EDIT",
			L"",
			WS_CHILD | ES_WANTRETURN | WS_VISIBLE | ES_CENTER,
			220,  // x padding
			270, // y padding
			50, // width
			30,  // height
			hWnd,
			(HMENU)IDC_MAIN_MCST_TTL_TEXTBOX,
			GetModuleHandle(NULL),
			NULL);

		// Manual IP connect button
		hDirectConnectButton = CreateWindowEx(NULL,
			L"BUTTON",
			L"Direct Connect",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			50,  // x padding
			320, // y padding
			120, // width
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
			190,  // x padding
			320, // y padding
			200, // width
			30,  // height
			hWnd,
			(HMENU)IDC_MAIN_IP_TEXTBOX,
			GetModuleHandle(NULL),
			NULL);

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

        // Put intialization code here

        // TODO: Add winsock initialization stuff here!
        InitWinsock();

        // Create broadcast listener
        bcst_lstn_sock = GetBroadcastSocket();
        if (bcst_lstn_sock != INVALID_SOCKET)
        {
            if (WSAAsyncSelect(bcst_lstn_sock, hWnd, WM_BCST_SOCKET, FD_READ))
            {
                wchar_t buffer[256];
                swprintf(buffer, 256, L"[Broadcast]: WSAAsyncSelect() failed with error: %i", WSAGetLastError());
                AddOutputMsg(buffer);
            }
        }

		// Create multicast listener
		mcst_lstn_sock = GetMulticastSocket();
		if (mcst_lstn_sock != INVALID_SOCKET)
		{
			if (WSAAsyncSelect(mcst_lstn_sock, hWnd, WM_MCST_SOCKET, FD_READ))
			{
                wchar_t buffer[256];
                swprintf(buffer, 256, L"[Multicast]: WSAAsyncSelect() failed with error: %i", WSAGetLastError());
                AddOutputMsg(buffer);
			}
		}

		// Set text length limit for IP box
		SendMessage(hIpBox, EM_LIMITTEXT, 15, NULL);

		setWindowFont(hOutputListBox);
		setWindowFont(hBroadcastButton);
		setWindowFont(hMulticastButton);
		setWindowFont(hIpBox);
		setWindowFont(hDirectConnectButton);

        break;
    case WM_BCST_SOCKET:
        switch (WSAGETSELECTEVENT(lParam))
        {
        case FD_READ:
            MessageBox(hWnd, L"Received a broadcast packet.", L"INFO", 0);
            ReceiveBroadcast(bcst_lstn_sock);
            break;
        }
        break;
    case WM_MCST_SOCKET:
		switch (WSAGETSELECTEVENT(lParam))
		{
		case FD_READ:
            MessageBox(hWnd, L"Received a multicast packet.", L"INFO", 0);
            if (mcst_lstn_sock != INVALID_SOCKET)
            {
                char * host = ReceiveMulticast(mcst_lstn_sock);
                conn_socket = ConnectTo(host, CONN_PORT, 3);

                if (conn_socket != INVALID_SOCKET)
                {
                    // Make window visible

                    if (WSAAsyncSelect(conn_socket, hPrimaryConnection, WM_MCST_SOCKET, FD_READ | FD_WRITE))
                    {
                        wchar_t buffer[256];
                        swprintf(buffer, 256, L"[Multicast]: WSAAsyncSelect() failed with error: %i", WSAGetLastError());
                        AddOutputMsg(buffer);
                    }
                    else
                    {
                        // Show connection window
                        SetWindowText(hPrimaryConnection, L"This is the new title!");
                        ShowWindow(hPrimaryConnection, 0);
                    }
                }
            }
            else
            {
                // Process a new client connection if not already connected
            }
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
