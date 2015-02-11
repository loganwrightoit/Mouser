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

const int MAX_LOADSTRING = 100;

// Global Variables:
HINSTANCE hInst;                     // current instance
float dpiScale = 1.0f;
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
HWND hPeerChatEditBox;
HWND hPeerChatButton;
HWND hPeerChatListBox;
HWND hPeerChatStreamButton;
HWND hPeerChatIsTypingLabel;
NetworkManager *network = &NetworkManager::getInstance();
PeerHandler *peerHandler = &PeerHandler::getInstance();

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    PeerWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    StreamWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
bool                isPeerChatSendCommand(MSG msg);
void                sendChatToPeer(HWND hWnd);
void setWindowFont(HWND hWnd);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	/*
		DPI-Awareness was introduced in Windows Vista, causing system metrics
		to return virtual windows properties by default.  Windows XP has
		no need for this, but Vista and beyond require calling this method to
		enable capturing actual screen dimensions.
	*/
	try {
		SetProcessDPIAware();
	}
	catch (exception) {}

    // Set program DPI scale
    HDC hdc = GetDC(GetDesktopWindow());
    dpiScale = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
    ReleaseDC(GetDesktopWindow(), hdc);

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
            // Detect ENTER command in peer chat editbox
            if (isPeerChatSendCommand(msg))
            {
                continue;
            }

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
    wMouserClass.hbrBackground = CreateSolidBrush(RGB(100, 150, 200));
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
    wPeerClass.hbrBackground = CreateSolidBrush(RGB(100, 150, 200));
    wPeerClass.lpszMenuName = MAKEINTRESOURCE(IDC_PEER);
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
    wStreamClass.hbrBackground = CreateSolidBrush(RGB(100, 150, 200));
    wStreamClass.lpszMenuName = MAKEINTRESOURCE(IDC_STREAM);
    wStreamClass.lpszClassName = szStreamClass;
    wStreamClass.hIconSm = NULL;

    return RegisterClassEx(&wMouserClass) &
           RegisterClassEx(&wPeerClass) &
           RegisterClassEx(&wStreamClass);
}

HWND getRootWindow()
{
    return hMouser;
}

bool isPeerChatSendCommand(MSG msg)
{
    // Detect ENTER send in peer chat editbox
    HWND hWnd = GetFocus();
    if (hWnd == hPeerChatEditBox)
    {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN)
        {
            sendChatToPeer(hWnd);
            return true;
        }
    }

    return false;
}

void sendChatToPeer(HWND hWnd)
{
    Peer* peer = peerHandler->getPeer(hWnd);
    if (peer != nullptr)
    {
        // Check if there is text to be sent to peer
        wchar_t text[MAX_CHAT_LENGTH];
        GetWindowText(hPeerChatEditBox, text, MAX_CHAT_LENGTH);
        if (wcslen(text) > 0)
        {            
            peer->sendChatMsg(text);

            // Clear text from control
            SetWindowText(GetDlgItem(peer->getRoot(), IDC_PEER_CHAT_EDITBOX), L"");
        }
    }
}

// Creates, shows, and updates window using WindowType
HWND getWindow(WindowType type, void* data = nullptr)
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
            500 * dpiScale,                 // width
            475 * dpiScale,                 // height
            NULL,                // hWndParent
            NULL,                // hMenu
            hInst,               // hInstance
            data);               // lpParam
        break;
    case PeerWin:
        hWnd = CreateWindowEx(
            WS_EX_CLIENTEDGE | WS_EX_ACCEPTFILES, // Accepts drag and drop
            szPeerClass,
            szPeerTitle,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            520 * dpiScale,
            380 * dpiScale,
            NULL,
            NULL,
            hInst,
            data);
        break;
    case StreamWin:
        hWnd = CreateWindowEx(
            WS_EX_CLIENTEDGE | WS_EX_ACCEPTFILES,
            szStreamClass,
            szStreamTitle,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            240 * dpiScale,
            120 * dpiScale,
            NULL,
            NULL,
            hInst,
            data);
        break;
    }

    setWindowFont(hWnd);
    centerWindow(hWnd);
    ShowWindow(hWnd, SW_SHOWNORMAL);
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
    if (!(hMouser = getWindow(WindowType::MouserWin)))
    {
        return FALSE;
    }

    return TRUE;
}

void AddOutputMsg(LPWSTR msg)
{
    int idx = SendMessage(hMouserOutputListBox, LB_ADDSTRING, 0, (LPARAM)msg);

    // Scroll to new message
    SendMessage(hMouserOutputListBox, LB_SETTOPINDEX, idx, 0);
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
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (msg)
    {
    case WM_CREATE:
        {
            // Set dimensions for window
            int width = 100 * dpiScale;
            RECT rect;
            if (GetWindowRect(hWnd, &rect))
            {
                width = rect.right - rect.left;
            }

            // Add peer label
            hMouserPeerLabel = CreateWindowEx(
                SS_CENTER,
                L"STATIC",
                NULL,
                WS_CHILD | WS_VISIBLE,
                5 * dpiScale,
                5 * dpiScale,
                (width - 5 * dpiScale) * 0.3F,
                30 * dpiScale,
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
                10 * dpiScale + width * 0.3F,
                5 * dpiScale,
                (width - 5 * dpiScale) * 0.7F - 30 * dpiScale,
                30 * dpiScale,
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
                5 * dpiScale,                            // X Padding
                30 * dpiScale,                           // Y Padding
                (width - 5 * dpiScale) * 0.3F,           // Width
                400 * dpiScale,                          // Height
                hWnd,                         // Parent window
                (HMENU)IDC_MAIN_PEER_LISTBOX,
                hInst,
                NULL);

            setWindowFont(hMouserPeerListBox);

            // Create output listbox
            hMouserOutputListBox = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                L"LISTBOX",
                NULL,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_AUTOVSCROLL,
                10 * dpiScale + width * 0.3F,
                30 * dpiScale,
                (width - 5 * dpiScale) * 0.7F - 30 * dpiScale,
                400 * dpiScale,
                hWnd,
                (HMENU)IDC_MAIN_OUTPUT_LISTBOX,
                hInst,
                NULL);

            setWindowFont(hMouserOutputListBox);

            // Initialize and setup connection-based and connectionless services
            NetworkManager::getInstance().init(hWnd);

            // Send out multicast to discover peers
            NetworkManager::getInstance().sendMulticast("Mouser|PEER_DISC");
        }
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
    case WM_EVENT_SEND_PACKET:
        {
            auto pair = (std::pair<Peer*, Packet*>*)wParam;
            pair->first->queuePacket(pair->second);
        }
        break;
    case WM_EVENT_OPEN_PEER_CHAT:
        return getWindow(WindowType::PeerWin, (Peer*)wParam) != NULL;
    case WM_EVENT_OPEN_PEER_STREAM:
        return getWindow(WindowType::StreamWin, (Peer*)wParam) != NULL;
    case WM_COMMAND:
        wmId = LOWORD(wParam);
        wmEvent = HIWORD(wParam);

        // Parse the menu selections:
        switch (wmId)
        {
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
        WSACleanup();
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

//
// Updates peer listbox when a peer connects or disconnects.
//
void updatePeerListBoxData()
{
    std::vector<Peer*> peers = peerHandler->getPeers();

    // Erase all peers from listbox
    SendMessage(hMouserPeerListBox, LB_RESETCONTENT, 0, 0);

    // Add updated peers to listbox
    auto iter = peers.begin();
    while (iter != peers.end())
    {
        // Only display name if it's known
        if (wcscmp((*iter)->getName(), L"Unknown") != 0)
        {
            // Add peer to listbox
            int index = SendMessage(hMouserPeerListBox, LB_ADDSTRING, 0, (LPARAM)(*iter)->getName());
            SendMessage(hMouserPeerListBox, LB_SETITEMDATA, (WPARAM)index, (LPARAM)*iter);
        }
        ++iter;
    }
}

UINT lastIsTypingTick = 0;

void CALLBACK hideChatIsTypingLabel(HWND hWnd, UINT msg, UINT timerId, DWORD dwTime)
{
    HWND isTypingLabel = GetDlgItem(hWnd, IDC_PEER_CHAT_IS_TYPING_LABEL);
    ShowWindow(isTypingLabel, SW_HIDE);
}

//
// Message handler for peer window.
//
LRESULT CALLBACK PeerWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Peer* peer;
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        peer = (Peer*)pCreate->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)peer);
        peer->setChatWindow(hWnd);
    }
    else
    {
        peer = (Peer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }

    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (msg)
    {
    case WM_CREATE:
        {
            int width = 100;
            RECT rect;
            if (GetWindowRect(hWnd, &rect))
            {
                width = rect.right - rect.left;
            }

            // Create output edit box
            hPeerChatListBox = CreateWindowEx(WS_EX_CLIENTEDGE,
                L"LISTBOX",
                L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL,
                5 * dpiScale,
                5 * dpiScale,
                width - 30 * dpiScale,
                280 * dpiScale,
                hWnd,
                (HMENU)IDC_PEER_CHAT_LISTBOX,
                hInst,
                NULL);

            setWindowFont(hPeerChatListBox);

            // Create send data button
            hPeerChatButton = CreateWindowEx(NULL,
                L"BUTTON",
                L"Send",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                5 * dpiScale,
                300 * dpiScale,
                width * 0.2F - 5,
                30 * dpiScale,
                hWnd,
                (HMENU)IDC_PEER_CHAT_BUTTON,
                hInst,
                NULL);

            setWindowFont(hPeerChatButton);

            // Create output edit box
            hPeerChatEditBox = CreateWindowEx(WS_EX_CLIENTEDGE,
                L"EDIT",
                L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL,
                (5 * dpiScale + (width - 10 * dpiScale) * 0.2F),
                300 * dpiScale,
                width * 0.6F - 25 * dpiScale,
                30 * dpiScale,
                hWnd,
                (HMENU)IDC_PEER_CHAT_EDITBOX,
                hInst,
                NULL);

            setWindowFont(hPeerChatEditBox);
            SendMessage(hPeerChatEditBox, EM_LIMITTEXT, MAX_CHAT_LENGTH, 0);

            // Create stream button
            hPeerChatStreamButton = CreateWindowEx(NULL,
                L"BUTTON",
                L"Stream",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                (width - 25 * dpiScale) * 0.8F,
                300 * dpiScale,
                width * 0.2F - 5 * dpiScale,
                30 * dpiScale,
                hWnd,
                (HMENU)IDC_PEER_CHAT_STREAM_BUTTON,
                hInst,
                NULL);

            setWindowFont(hPeerChatStreamButton);

            hPeerChatIsTypingLabel = CreateWindowEx(NULL,
                L"STATIC",
                L"Peer is typing...",
                WS_CHILD,
                5 * dpiScale,
                280 * dpiScale,
                width - 20 * dpiScale,
                20 * dpiScale,
                hWnd,
                (HMENU)IDC_PEER_CHAT_IS_TYPING_LABEL,
                hInst,
                NULL);

            setWindowFont(hPeerChatIsTypingLabel);

        }
        break;
    case WM_COMMAND:
        wmId = LOWORD(wParam);
        wmEvent = HIWORD(wParam);

        switch (wmId)
        {
            case IDC_PEER_CHAT_BUTTON:
                sendChatToPeer(hWnd);
                break;
            case IDC_PEER_CHAT_EDITBOX:
                // If real-time chat is enabled, need to send text here on keypress
                // For now, it's only "Peer is typing..." indicator
                {
                    UINT ticks = GetTickCount();
                    if ((ticks - lastIsTypingTick) > 500)
                    {
                        peer->sendPacket(new Packet(Packet::CHAT_IS_TYPING));
                        lastIsTypingTick = ticks;
                    }
                }
                break;
            case IDC_PEER_CHAT_LISTBOX:
                // Listbox is passive, nothing really to do here
                break;
            case IDC_PEER_CHAT_STREAM_BUTTON:
                peer->streamTo();
                break;
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        peer->onDestroyRoot();
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}

//
// Message handler for stream window.
//
LRESULT CALLBACK StreamWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Peer* peer;
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        peer = (Peer*)pCreate->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)peer);
        peer->setStreamWindow(hWnd);
    }
    else
    {
        peer = (Peer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }

    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (msg)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        peer->sendPacket(new Packet(Packet::STREAM_STOP));
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}

//
// Message handler for about box.
//
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
