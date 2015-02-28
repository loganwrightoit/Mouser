// Author: Logan Wright
// Mouser.cpp : Defines the entry point for the application.
//

#pragma once

#include "stdafx.h"
#include "Mouser.h"
#include <string>
#include <stdlib.h>
#include <atlimage.h> // IStream_ and CImage functions
#include "Shellapi.h" // WM_DROPFILES
#include <fstream>

using namespace std;

const int MAX_LOADSTRING = 100;

// Global Variables:
HINSTANCE hInst;                     // current instance
float dpiScale = 1.0f;
wchar_t _username[UNLEN + 1]; // User name
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
HWND hMouserPeerConnectButton;
HWND hMouserPeerConnectEditBox;
HWND hMouserOutputLabel;
NetworkManager *network = &NetworkManager::getInstance();
PeerHandler *peerHandler = &PeerHandler::getInstance();
HBRUSH brushBgnd = CreateSolidBrush(RGB(100, 150, 200));

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    PeerWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    StreamWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
bool                isPeerChatSendCommand(MSG msg);
void                sendChatToPeer(HWND hWnd);
void                setWindowFont(HWND hWnd);

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

	// Set username
	DWORD szName = sizeof(_username);
	if (!GetUserName(_username, &szName))
	{
		wcscpy_s(_username, UNLEN + 1, L"UnknownPeer");
	}

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

    wMouserClass.cbSize = sizeof(WNDCLASSEX);
    wMouserClass.style = CS_HREDRAW | CS_VREDRAW;
    wMouserClass.lpfnWndProc = MainWndProc;
    wMouserClass.cbClsExtra = NULL;
    wMouserClass.cbWndExtra = NULL;
    wMouserClass.hInstance = hInstance;
    wMouserClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MOUSER));
    wMouserClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wMouserClass.hbrBackground = brushBgnd;
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
	wPeerClass.hbrBackground = brushBgnd;
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
	wStreamClass.hbrBackground = brushBgnd;
    wStreamClass.lpszMenuName = MAKEINTRESOURCE(IDC_STREAM);
    wStreamClass.lpszClassName = szStreamClass;
    wStreamClass.hIconSm = NULL;

    return RegisterClassEx(&wMouserClass) &
           RegisterClassEx(&wPeerClass) &
           RegisterClassEx(&wStreamClass);
}

HBRUSH getDefaultBrush()
{
    return brushBgnd;
}

wchar_t* getUserName()
{
	return _username;
}

HWND getRootWindow()
{
    return hMouser;
}

bool isPeerChatSendCommand(MSG msg)
{
    // Detect ENTER send in peer chat editbox
    HWND hWnd = GetFocus();
    int id = GetDlgCtrlID(hWnd);

    if (GetDlgCtrlID(hWnd) == IDC_PEER_CHAT_EDITBOX)
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
        GetWindowText(peer->hChatEditBox, text, MAX_CHAT_LENGTH);
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
            WS_EX_CLIENTEDGE,     // extended styles
            szMouserClass,        // lpClassName
            szMouserTitle,        // lpWindowName,
            WS_OVERLAPPEDWINDOW,  // dwStyle
            CW_USEDEFAULT,        // x
            CW_USEDEFAULT,        // y
            (int) (500 * dpiScale), // width
            (int) (475 * dpiScale), // height
            NULL,                 // hWndParent
            NULL,                 // hMenu
            hInst,                // hInstance
            data);                // lpParam
        break;
    case PeerWin:
        hWnd = CreateWindowEx(
            WS_EX_CLIENTEDGE | WS_EX_ACCEPTFILES, // Accepts drag and drop
            szPeerClass,
            szPeerTitle,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
			(int) (520 * dpiScale),
			(int) (380 * dpiScale),
            NULL,
            NULL,
            hInst,
            data);
        break;
    case StreamWin:
        hWnd = CreateWindowEx(
            WS_EX_CLIENTEDGE | WS_EX_ACCEPTFILES, // Accepts drag and drop
            szStreamClass,
            szStreamTitle,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
			(int) (240 * dpiScale),
			(int) (120 * dpiScale),
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
void centerWindow(HWND hWnd, RECT newSize)
{
    int scrX = GetSystemMetrics(SM_CXSCREEN);
    int scrY = GetSystemMetrics(SM_CYSCREEN);

    RECT rect;
    if (newSize.left > -1)
    {
        rect = newSize;
    }
    else
    {
        GetWindowRect(hWnd, &rect);
    }

    int winWidth = rect.right - rect.left;
    int winHeight = rect.bottom - rect.top;
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
            int width = (int) (100 * dpiScale);
            RECT rect;
            if (GetWindowRect(hWnd, &rect))
            {
                width = rect.right - rect.left;
            }

			// Add peer direct connect button
			hMouserPeerConnectButton = CreateWindowEx(
				NULL,
				L"BUTTON",
				L"Find Peer by IP",
				WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
				(int)(5 * dpiScale),
				(int)(5 * dpiScale),
				(int)((width - 5 * dpiScale) * 0.3F),
				(int)(30 * dpiScale),
				hWnd,
				(HMENU)IDC_MAIN_PEER_BUTTON,
				hInst,
				NULL);

			setWindowFont(hMouserPeerConnectButton);

			// Add peer direct connect edtibox
			hMouserPeerConnectEditBox = CreateWindowEx(
				WS_EX_CLIENTEDGE,
				L"EDIT",
				L"",
				WS_CHILD | WS_VISIBLE | WS_VSCROLL,
				(int)(5 * dpiScale),
				(int)(35 * dpiScale),
				(int)((width - 5 * dpiScale) * 0.3F),
				(int)(30 * dpiScale),
				hWnd,
				(HMENU)IDC_MAIN_PEER_EDITBOX,
				hInst,
				NULL);

			setWindowFont(hMouserPeerConnectEditBox);

            // Add peer label
            hMouserPeerLabel = CreateWindowEx(
                SS_CENTER,
                L"STATIC",
                NULL,
                WS_CHILD | WS_VISIBLE,
                (int)(5 * dpiScale),
				(int)(70 * dpiScale),
				(int)((width - 5 * dpiScale) * 0.3F),
				(int)(30 * dpiScale),
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
                (int)(10 * dpiScale + width * 0.3F),
				(int)(5 * dpiScale),
				(int)((width - 5 * dpiScale) * 0.7F - 30 * dpiScale),
				(int)(30 * dpiScale),
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
				(int)(5 * dpiScale),                            // X Padding
				(int)(100 * dpiScale),                           // Y Padding
				(int)((width - 5 * dpiScale) * 0.3F),           // Width
				(int)(330 * dpiScale),                          // Height
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
				(int)(10 * dpiScale + width * 0.3F),
				(int)(30 * dpiScale),
				(int)((width - 5 * dpiScale) * 0.7F - 30 * dpiScale),
				(int)(400 * dpiScale),
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
		case IDC_MAIN_PEER_BUTTON:
			{
				// Get IP from editbox
				wchar_t buffer[16];
				GetWindowText(hMouserPeerConnectEditBox, buffer, sizeof(buffer));

				if (wcslen(buffer) == 0)
				{
					AddOutputMsg(L"[P2P]: Must enter a valid IP address.");
				}
				else
				{
					// Convert wchar_t* string to char*
					size_t szText = wcslen(buffer) + 1; // +1 for null terminator
					char* text = new char[szText];
					size_t charsConverted = 0;
					wcstombs_s(&charsConverted, text, szText, buffer, wcslen(buffer));

					// Attempt peer connection
					peerHandler->directConnectToPeer(text);

					// Clean up memory
					delete[] text;
				}
			}
			break;
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

WNDPROC wpOrigEditProc;

// Subclass procedure 
LRESULT APIENTRY EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Peer* peer = (Peer*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_KEYDOWN: // Naturally ignores ENTER keypress
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
    }

    return CallWindowProc(wpOrigEditProc, hwnd, uMsg, wParam, lParam);
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

            // Create output listbox
            peer->hChatListBox = CreateWindowEx(WS_EX_CLIENTEDGE,
                L"LISTBOX",
                L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL,
				(int)(5 * dpiScale),
				(int)(5 * dpiScale),
				(int)(width - 30 * dpiScale),
				(int)(280 * dpiScale),
                hWnd,
                (HMENU)IDC_PEER_CHAT_LISTBOX,
                hInst,
                NULL);

            setWindowFont(peer->hChatListBox);

            // Create send data button
            peer->hChatButton = CreateWindowEx(NULL,
                L"BUTTON",
                L"Send",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
				(int)(5 * dpiScale),
				(int)(300 * dpiScale),
				(int)(width * 0.2F - 5),
				(int)(30 * dpiScale),
                hWnd,
                (HMENU)IDC_PEER_CHAT_BUTTON,
                hInst,
                NULL);

            setWindowFont(peer->hChatButton);

            // Create input editbox
            peer->hChatEditBox = CreateWindowEx(WS_EX_CLIENTEDGE,
                L"EDIT",
                L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL,
				(int)((5 * dpiScale + (width - 10 * dpiScale) * 0.2F)),
				(int)(300 * dpiScale),
				(int)(width * 0.6F - 25 * dpiScale),
				(int)(30 * dpiScale),
                hWnd,
                (HMENU)IDC_PEER_CHAT_EDITBOX,
                hInst,
                NULL);

            // Subclass the edit control.
            wpOrigEditProc = (WNDPROC)SetWindowLong(peer->hChatEditBox, GWL_WNDPROC, (LONG)EditSubclassProc);
            SetWindowLongPtr(peer->hChatEditBox, GWLP_USERDATA, (LONG_PTR)peer);

            setWindowFont(peer->hChatEditBox);
            SendMessage(peer->hChatEditBox, EM_LIMITTEXT, MAX_CHAT_LENGTH, 0);

            // Create stream button
            peer->hChatStreamButton = CreateWindowEx(
                NULL,
                L"BUTTON",
                L"Share Screen",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
				(int)((width - 25 * dpiScale) * 0.8F),
				(int)(300 * dpiScale),
				(int)(width * 0.2F - 5 * dpiScale),
				(int)(30 * dpiScale),
                hWnd,
                (HMENU)IDC_PEER_CHAT_STREAM_BUTTON,
                hInst,
                NULL);

            setWindowFont(peer->hChatStreamButton);

            // Create "Peer is typing..." label
            peer->hChatIsTypingLabel = CreateWindowEx(NULL,
                L"STATIC",
                L"Peer is typing...",
                WS_CHILD,
				(int)(5 * dpiScale),
				(int)(280 * dpiScale),
				(int)(width - 20 * dpiScale),
				(int)(20 * dpiScale),
                hWnd,
                (HMENU)IDC_PEER_CHAT_IS_TYPING_LABEL,
                hInst,
                NULL);

            setWindowFont(peer->hChatIsTypingLabel);
        }
        break;
	case WM_CTLCOLORSTATIC:
		{
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(255, 255, 255));
			SetBkMode(hdcStatic, TRANSPARENT);

			return (LRESULT)brushBgnd;
		}
    case WM_DROPFILES:
        {
            HDROP hDrop = (HDROP)wParam;
            UINT numFiles = DragQueryFile(hDrop, 0xffffffff, NULL, NULL);
            
            // Only allow single file
            if (numFiles == 1)
            {
                wchar_t lpszFile[MAX_PATH];
                if (DragQueryFile(hDrop, 0, lpszFile, MAX_PATH))
                {
                    peer->makeFileSendRequest(lpszFile);
                }
            }

            DragFinish(hDrop);
            break;
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
        case IDC_PEER_CHAT_LISTBOX:
            // Listbox is passive, nothing really to do here
            break;
        case IDC_PEER_CHAT_STREAM_BUTTON:
            peer->makeStreamRequest(GetDesktopWindow());
            break;
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        // Remove the subclass from the edit control. 
        SetWindowLong(peer->hChatEditBox, GWL_WNDPROC, (LONG)wpOrigEditProc);
        peer->onDestroyRoot();
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}

bool noRedraw = false;

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

    switch (msg)
    {
    case WM_SIZING:
        {
            /*
            RECT rect;
            memcpy_s(&rect, sizeof(rect), (RECT*)lParam, sizeof(rect));

            rect.right = rect.right - rect.left;
            rect.bottom = rect.bottom - rect.top;
            rect.left = rect.top = 0;

            peer->streamResize(rect);
            */
        }
        break;
    case WM_ERASEBKGND:
        return FALSE;
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            // Refresh stream image in updated region
            peer->DrawStreamImage(hdc, ps.rcPaint);

            // Draw cursor in current location
            peer->DrawStreamCursor(hdc, ps.rcPaint);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_CLOSE:
        peer->sendPacket(new Packet(Packet::STREAM_CLOSE));
        DestroyWindow(hWnd);
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
