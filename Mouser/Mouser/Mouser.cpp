// Author: Logan Wright
// Mouser.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Mouser.h"
#include <string>

using namespace std;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                        // current instance
TCHAR szTitle[MAX_LOADSTRING];          // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];    // the main window class name
HWND hOutputListBox;
SOCKET mcst_lstn_sock = INVALID_SOCKET, bcst_lstn_sock = INVALID_SOCKET;

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
	HWND hBroadcastButton, hMulticastButton, hMulticastTTLBox, hIpBox = NULL, hDirectConnectButton;

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

        // Put intialization code here

        // TODO: Add winsock initialization stuff here!
        InitWinsock();

        // Create broadcast listener
        bcst_lstn_sock = GetBroadcastSocket();
        if (bcst_lstn_sock != INVALID_SOCKET)
        {
            if (WSAAsyncSelect(bcst_lstn_sock, hWnd, WM_BCST_SOCKET, FD_READ))
            {
                //AddOutputMsg((LPWSTR)result);
                MessageBox(hWnd, L"WSAAsyncSelect() failed for broadcast socket.", L"Critical Error", MB_ICONERROR);
            }
        }

		// Create multicast listener
		mcst_lstn_sock = GetMulticastSocket();
		if (mcst_lstn_sock != INVALID_SOCKET)
		{
			if (WSAAsyncSelect(mcst_lstn_sock, hWnd, WM_MCST_SOCKET, FD_READ))
			{
				//AddOutputMsg((LPWSTR)result);
				MessageBox(hWnd, L"WSAAsyncSelect() failed for multicast socket.", L"Critical Error", MB_ICONERROR);
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
            ReceiveBroadcast(bcst_lstn_sock);
            break;
        }
        break;
    case WM_MCST_SOCKET:
		switch (WSAGETSELECTEVENT(lParam))
		{
		case FD_READ:
			ReceiveMulticast(mcst_lstn_sock);
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
			// Set TTL
			//SendMessage(hMulticastTTLBox, WM_GETTEXT, (WPARAM)10, (LPARAM)ttl);
			//SetMulticastTTL(mcst_lstn_sock, ttl);
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

        // Close sockets and WSA
        //closesocket(bcst_lstn_sock);
        //closesocket(mcst_lstn_sock);
        //WSACleanup();

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
