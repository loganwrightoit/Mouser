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
HINSTANCE hInst;                     // current instance
TCHAR szMouserTitle[MAX_LOADSTRING];   // The title bar text
TCHAR szPeerTitle[MAX_LOADSTRING];   // The title bar text
TCHAR szStreamTitle[MAX_LOADSTRING]; // The title bar text
TCHAR szMouserClass[MAX_LOADSTRING];   // the main window class name
TCHAR szPeerClass[MAX_LOADSTRING];   // the main window class name
TCHAR szStreamClass[MAX_LOADSTRING]; // the main window class name
HWND hMouser;
HWND hMouserOutputListBox;
HWND hMouserPeerListBox;
HWND hMouserPeerLabel;
HWND hMouserOutputLabel;
NetworkManager *network = &NetworkManager::getInstance();
PeerHandler *peerHandler = &PeerHandler::getInstance();

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    PeerWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    StreamWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void setWindowFont(HWND hWnd);
void centerWindow(HWND hWnd);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MSG msg;
    HACCEL hAccelTable;

    // Initialize global strings
    LoadString(hInstance, IDS_MOUSER_TITLE, szMouserTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_MOUSER, szMouserClass, MAX_LOADSTRING);
    LoadString(hInstance, IDS_PEER_TITLE, szPeerTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_PEER, szPeerClass, MAX_LOADSTRING);
    LoadString(hInstance, IDS_STREAM_TITLE, szStreamTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_STREAM, szStreamClass, MAX_LOADSTRING);
    
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
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

    return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wMouserClass;
    WNDCLASSEX wPeerClass;
    WNDCLASSEX wStreamClass;

    // May be unnecessary
    memset(&wMouserClass, 0, sizeof(WNDCLASSEX));
    memset(&wPeerClass, 0, sizeof(WNDCLASSEX));
    memset(&wStreamClass, 0, sizeof(WNDCLASSEX));

    wMouserClass.cbSize = sizeof(WNDCLASSEX);
    wMouserClass.style = CS_HREDRAW | CS_VREDRAW;
    wMouserClass.lpfnWndProc = MainWndProc;
    wMouserClass.cbClsExtra = NULL;
    wMouserClass.cbWndExtra = NULL;
    wMouserClass.hInstance = hInstance;
    wMouserClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MOUSER));
    wMouserClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wMouserClass.hbrBackground = CreateSolidBrush(RGB(100, 150, 200)); // Default: (HBRUSH)(COLOR_WINDOW+1)
    wMouserClass.lpszMenuName = MAKEINTRESOURCE(IDC_MOUSER);
    wMouserClass.lpszClassName = szMouserClass;
    wMouserClass.hIconSm = LoadIcon(wMouserClass.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    // Peer window
    wPeerClass.cbSize = sizeof(WNDCLASSEX);
    wPeerClass.style = CS_HREDRAW | CS_VREDRAW;
    wPeerClass.lpfnWndProc = PeerWndProc;
    wPeerClass.cbClsExtra = NULL;
    wPeerClass.cbWndExtra = NULL;
    wPeerClass.hInstance = hInstance;
    wPeerClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MOUSER));
    wPeerClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wPeerClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wPeerClass.lpszMenuName = NULL;
    wPeerClass.lpszClassName = szPeerClass;
    wPeerClass.hIconSm = NULL;

    // Stream window
    wStreamClass.cbSize = sizeof(WNDCLASSEX);
    wStreamClass.style = CS_HREDRAW | CS_VREDRAW;
    wStreamClass.lpfnWndProc = StreamWndProc;
    wStreamClass.cbClsExtra = NULL;
    wStreamClass.cbWndExtra = NULL;
    wStreamClass.hInstance = hInstance;
    wStreamClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MOUSER));
    wStreamClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wStreamClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wStreamClass.lpszMenuName = NULL;
    wStreamClass.lpszClassName = szStreamClass;
    wStreamClass.hIconSm = NULL;

    return RegisterClassEx(&wMouserClass) &
           RegisterClassEx(&wPeerClass) &
           RegisterClassEx(&wStreamClass);
}

// Creates, shows, and updates window using WindowType
HWND getWindow(WindowType type, int nCmdShow)
{
    HWND hWnd;

    // NOTE ON FILE SENDING
    // WS_EX_ACCEPTFILES used as first parameter of CreateWindowEx enables drag-and-drop functionality
    // Look here for more flags: https://msdn.microsoft.com/en-us/library/windows/desktop/ff700543%28v=vs.85%29.aspx

    switch (type)
    {
    case MouserWin:
        hWnd = CreateWindowEx(
            WS_EX_CLIENTEDGE,    // extended styles
            szMouserClass,       // lpClassName
            szMouserTitle,       // lpWindowName,
            WS_OVERLAPPEDWINDOW, // dwStyle
            CW_USEDEFAULT,       // x
            CW_USEDEFAULT,       // y
            500,                 // width
            475,                 // height
            NULL,                // hWndParent
            NULL,                // hMenu
            hInst,               // hInstance
            NULL);               // lpParam
        break;
    case PeerWin:
        hWnd = CreateWindowEx(
            WS_EX_CLIENTEDGE | WS_EX_ACCEPTFILES, // Accepts drag and drop
            szPeerClass,
            szPeerTitle,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            300,
            390,
            NULL,
            NULL,
            hInst,
            NULL);
        break;
    case StreamWin:
        hWnd = CreateWindowEx(
            WS_EX_CLIENTEDGE | WS_EX_ACCEPTFILES,
            szStreamClass,
            szStreamTitle,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            240,
            120,
            NULL,
            NULL,
            hInst,
            NULL);
        break;
    }

    setWindowFont(hWnd);
    centerWindow(hWnd);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return hWnd;
}

//
// Centers window.
//
void centerWindow(HWND hWnd)
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
// Sets window font
//
void setWindowFont(HWND hWnd)
{
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
    HFONT hFont = ::CreateFontIndirect(&ncm.lfMessageFont);
    SendMessage(hWnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
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

    // Create main window
    if (!(hMouser = getWindow(WindowType::MouserWin, nCmdShow)))
    {
        return FALSE;
    }

    return TRUE;
}

void AddOutputMsg(LPWSTR msg)
{
    SendMessage(hMouserOutputListBox, LB_ADDSTRING, 0, (LPARAM)msg);
}

/*
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
*/

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

    int width = 100;
    RECT rect;
    if (GetWindowRect(hWnd, &rect))
    {
        width = rect.right - rect.left;
    }

    switch (msg)
    {
    case WM_CREATE:

        // Add peer label
        hMouserPeerLabel = CreateWindowEx(
            SS_CENTER,
            L"STATIC",
            NULL,
            WS_CHILD | WS_VISIBLE,
            5,
            5,
            (width - 5) * 0.3F,
            30,
            hWnd,
            NULL,
            hInst,
            NULL);

        SetWindowTextW(hMouserPeerLabel, L"Peers");

        // Add output label
        hMouserOutputLabel = CreateWindowEx(
            SS_CENTER,
            L"STATIC",
            NULL,
            WS_CHILD | WS_VISIBLE,
            10 + width * 0.3F,
            5,
            (width - 5) * 0.7F - 30,
            30,
            hWnd,
            NULL,
            hInst,
            NULL);

        SetWindowTextW(hMouserOutputLabel, L"Output");

        // Create listbox for peers
        hMouserPeerListBox = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            L"LISTBOX",
            NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_AUTOVSCROLL | LBS_HASSTRINGS | LBS_NOTIFY,
            5,                            // X Padding
            30,                           // Y Padding
            (width - 5) * 0.3F,           // Width
            400,                          // Height
            hWnd,                         // Parent window
            (HMENU)IDC_MAIN_PEER_LISTBOX,
            hInst,
            NULL);

        setWindowFont(hMouserPeerListBox);

        // Create output edit box
        hMouserOutputListBox = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            L"LISTBOX",
            NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_AUTOVSCROLL,
            10 + width * 0.3F,
            30,
            (width - 5) * 0.7F - 30,
            400,
            hWnd,
            (HMENU)IDC_MAIN_OUTPUT_LISTBOX,
            hInst,
            NULL);

        setWindowFont(hMouserOutputListBox);

        // Initialize and setup connection-based and connectionless services
        NetworkManager::getInstance().init(hWnd);

        // Send out multicast to discover peers
        NetworkManager::getInstance().sendMulticast("Mouser|PEER_DISC");

        break;
    case WM_MCST_SOCKET:
        switch (WSAGETSELECTEVENT(lParam))
        {
            case FD_READ:
                peerHandler->connectToPeer();
                break;
        }
        break;
    case WM_P2P_LISTEN_SOCKET:
        switch (WSAGETSELECTEVENT(lParam))
        {
            case FD_ACCEPT:
                peerHandler->handlePeerConnectionRequest(wParam);
                break;
        }
        break;
    case WM_COMMAND:
        wmId = LOWORD(wParam);
        wmEvent = HIWORD(wParam);

        // Parse the menu selections:
        switch (wmId)
        {
            /*
        case IDC_MAIN_CAPTURE_SCREEN_BUTTON:
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
            break;
            */
            /*
        case IDC_MAIN_SEND_PEER_DATA_BUTTON:
            if (peerHandler->getDefaultPeer() != nullptr)
            {
                peerHandler->getDefaultPeer()->sendStreamCursor();
            }
            break;
            */
        case IDC_MAIN_PEER_LISTBOX:
            switch (wmEvent)
            {
                case LBN_DBLCLK:
                    {
                        HWND hwndList = GetDlgItem(hWnd, IDC_MAIN_PEER_LISTBOX);

                        // Get selected index.
                        int idx = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0);

                        // Get item data.
                        Peer* peer = (Peer*)SendMessage(hwndList, LB_GETITEMDATA, idx, 0);

                        peer->openChatWindow();
                    }
                    break;
            }
            break;
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
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

void updatePeerListBoxData()
{
    std::vector<Peer*> peers = peerHandler->getPeers();

    // Erase all peers from listbox
    SendMessage(hMouserPeerListBox, LB_RESETCONTENT, 0, 0);

    // Add updated peers to listbox
    auto iter = peers.begin();
    while (iter != peers.end())
    {
        // Gather peer info
        sockaddr_in addr;
        int size = sizeof(addr);
        getpeername((*iter)->getSocket(), (sockaddr*)&addr, &size);
        char* ip = inet_ntoa(addr.sin_addr);

        // Get peer string identifier
        const size_t szStr = strlen(ip) + 1;
        std::wstring ident(szStr, L'#');
        mbstowcs(&ident[0], ip, szStr);

        // Add peer to listbox
        int index = SendMessage(hMouserPeerListBox, LB_ADDSTRING, 0, (LPARAM)ident.c_str());
        SendMessage(hMouserPeerListBox, LB_SETITEMDATA, (WPARAM)index, (LPARAM)*iter);

        ++iter;
    }
}

// Message handler for peer window.
LRESULT CALLBACK PeerWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (msg)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
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
        return FALSE;
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
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
