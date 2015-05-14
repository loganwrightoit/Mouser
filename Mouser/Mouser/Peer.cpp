#include "stdafx.h"
#include "Peer.h"
#include <thread>
#include <string>
#include "math.h"
#include "ImageUtil.h"
#include "Shellapi.h"

#define NOMINMAX

Peer::Peer(SOCKET peer_socket = 0)
: _socket(peer_socket), _hWnd(0), _hWnd_stream(0), _streamSender(0), _cursorUtil(0), _fileSender(0)
{
    _name = L"Unknown";

    // Create worker to handle socket sends
    _worker = new Worker(_socket);

    // Start receive thread
    std::thread rt(&Peer::rcvThread, this);
    rt.detach();

    HANDLE readyEvent = _worker->getReadyEvent();
    DWORD dwWaitResult = WaitForSingleObject(readyEvent, INFINITE);
    if (dwWaitResult != WAIT_OBJECT_0)
    {
        return;
    }
    _worker->setReady();
    CloseHandle(readyEvent);

    // Send host name to peer
    sendName();
}

Peer::~Peer()
{
    // Close socket
    shutdown(_socket, SD_BOTH);
    closesocket(_socket);

    delete _worker;
}

HWND Peer::getStream()
{
    return _hWnd_stream;
}

void Peer::setChatWindow(HWND hWnd)
{
    _hWnd = hWnd;
}

void Peer::setStreamWindow(HWND hWnd)
{
    _hWnd_stream = hWnd;
}

void Peer::onDestruct()
{
    if (_hWnd_stream)
    {
        if (_streamSender)
        {
            _streamSender->stop();
            delete _streamSender;
        }
        if (_cursorUtil)
        {
            _cursorUtil->stop();
            delete _cursorUtil;
        }
        DestroyWindow(_hWnd_stream);
    }
    if (_hWnd)
    {
        DestroyWindow(_hWnd);
    }
}

void Peer::setInputFocus()
{
    HWND editCtrl = GetDlgItem(_hWnd, IDC_PEER_CHAT_EDITBOX);
    SetFocus(editCtrl);
    SendDlgItemMessage(editCtrl, IDC_PEER_CHAT_EDITBOX, EM_SETSEL, 0, -1);
}

void Peer::openChatWindow()
{
    // If window exists, make active
    if (_hWnd != NULL)
    {
        ShowWindow(_hWnd, SW_RESTORE);
        SetFocus(_hWnd);
    }
    else
    {
        if (SendMessage(getRootWindow(), WM_EVENT_OPEN_PEER_CHAT, (WPARAM)this, NULL))
        {
            // Update window title with peer name
            SetWindowText(_hWnd, _name);

            // Update "Peer is typing..." label with peer name
            std::wstring str = _name;
            str.append(L" is typing...");
            HWND hWnd = GetDlgItem(_hWnd, IDC_PEER_CHAT_IS_TYPING_LABEL);
            SendMessage(hWnd, WM_SETTEXT, NULL, (LPARAM)str.c_str());
        }
    }

    // Focus on input text area
    setInputFocus();
}

Worker* Peer::getWorker()
{
    return _worker;
}

void Peer::openStreamWindow()
{
    // If window exists, make active
    if (_hWnd_stream != NULL)
    {
        ShowWindow(_hWnd_stream, SW_RESTORE);
        SetFocus(_hWnd_stream);
    }
    else
    {
        SendMessage(getRootWindow(), WM_EVENT_OPEN_PEER_STREAM, (WPARAM)this, NULL);
    }
}

SOCKET Peer::getSocket() const
{
    return _socket;
}

void Peer::addChat(LPWSTR msg)
{
    openChatWindow();

    // Find peer chat output box
    HWND hWnd = GetDlgItem(_hWnd, IDC_PEER_CHAT_LISTBOX);
    if (hWnd != NULL)
    {
        int idx = SendMessage(hWnd, LB_ADDSTRING, 0, (LPARAM)msg);

        // Scroll to new message
        SendMessage(hWnd, LB_SETTOPINDEX, idx, 0);
    }
}

void Peer::sendName()
{
    std::pair<char*, size_t> buffer;
    
    if (encode_utf8(&buffer, getUserName()))
    {
        // Send name to peer
        _worker->sendPacket(new Packet(Packet::NAME, buffer.first, buffer.second));
    }
}

void Peer::makeFileSendRequest(wchar_t* path)
{
    // Open file and seek to end (ate) to get size
    std::ifstream file(path, std::ifstream::binary | std::ifstream::in | std::ifstream::ate);

    if (file.is_open())
    {
        // Populate file info
        FileSender::FileInfo info;
        wcscpy_s(info.path, wcslen(path) + 1, path);
        info.size = (size_t)file.tellg();

        // Create file sender object (creates file transfer window)
        _fileSender = new FileSender(this, TRUE, info);

        // Send file info to peer (filename, size)
        char* data = new char[sizeof(info)];
        memcpy_s(data, sizeof(info), &info, sizeof(info));
        _worker->sendPacket(new Packet(Packet::FILE_SEND_REQUEST, data, sizeof(info)));

        // Close file
        file.close();
    }
}

void Peer::stopSharing()
{
    _streamSender->stop();
    _cursorUtil->stop();
    EnableMenuItem(_menu, 0, MF_ENABLED | MF_BYPOSITION);
    EnableWindow(hChatStopSharingButton, false);
    DrawMenuBar(_hWnd);
}

void Peer::makeStreamRequest(HWND hWnd)
{
    EnableMenuItem(_menu, 0, MF_GRAYED | MF_BYPOSITION);
    DrawMenuBar(_hWnd);

    // Store source HWND for later
    _hWnd_stream_src = hWnd;
        
    // Send stream info (window size, title)
    StreamSender::StreamInfo info;
    RECT rect;
    GetWindowRect(hWnd, &rect);
    info.width = rect.right - rect.left;
    info.height = rect.bottom - rect.top;
    info.bpp = 32; // Hardcoded, but is a typical value
    
    if (!GetWindowText(hWnd, info.name, 256))
    {
        if (hWnd == GetDesktopWindow())
        {
            wcscpy_s(info.name, sizeof(info.name), L"Desktop");
        }
        else
        {
            wcscpy_s(info.name, sizeof(info.name), L"Unknown Screen");
        }
    }

    // Save values to stream sender for later
    _streamSender = new StreamSender(this, info);

    // Send stream request packet
    char * data = new char[sizeof(info)];

    // Not zero is error condition
    if (memcpy_s(data, sizeof(info), &info, sizeof(info)))
    {
        printf("[DEBUG]: StreamInfo memcpy_s failed, aborting.\n");
        delete[] data;
        return;
    }

    _worker->sendPacket(new Packet(Packet::STREAM_REQUEST, data, sizeof(info)));
}

int Peer::displayFileSendRequestMessageBox(FileSender::FileInfo info)
{
    wchar_t fileName[MAX_PATH];
    wcscpy_s(fileName, MAX_PATH, info.path);
    PathStripPath(fileName);

    wchar_t buffer[256];
    swprintf(buffer, 256, L"%ls wants to send you a file:\n\n%ls (%i bytes)\n\nDo you want to accept it?", _name, fileName, info.size);

    int msgboxID = MessageBox(
        NULL,
        buffer,
        L"Accept File Send",
        MB_ICONEXCLAMATION | MB_YESNO
        );

    return msgboxID;
}

int Peer::displayStreamRequestMessageBox(wchar_t* name)
{
    wchar_t buffer[256];
    swprintf(buffer, 256, L"%ls wants to share a screen with you:\n\n%ls\n\nDo you want to accept it?", _name, name);

    int msgboxID = MessageBox(
        NULL,
        buffer,
        L"Accept Screen Share",
        MB_ICONEXCLAMATION | MB_YESNO
        );

    return msgboxID;
}

FileSender* Peer::getFileSender() const
{
    return _fileSender;
}

void Peer::openDownloadDialog()
{
    if (!_fileSender)
    {
        _fileSender = new FileSender(this, FALSE, _temp);
    }
}

void Peer::getFileSendRequest(Packet* pkt)
{
    // Populate temporary file info
    if (memcpy_s(&_temp, sizeof(struct FileSender::FileInfo), pkt->getData(), sizeof(struct FileSender::FileInfo)))
    {
        return; // Not zero is error condition
    }

    if (displayFileSendRequestMessageBox(_temp) == IDYES) // Peer accepted file transfer request
    {
        SendMessage(getRootWindow(), WM_EVENT_CREATE_DOWNLOADBOX, (WPARAM)this, NULL);
    }
    else
    {
        _worker->sendPacket(new Packet(Packet::FILE_SEND_DENY));
    }
}

void Peer::getFileSendAllow(Packet* pkt)
{
    _fileSender->onPeerAcceptRequest();
}

void Peer::getFileSendDeny(Packet* pkt)
{
    _fileSender->onPeerRejectRequest();
}

void Peer::rcvThread()
{
    // Gather peer info
    sockaddr_in addr;
    int size = sizeof(addr);
    getpeername(_socket, (sockaddr*)&addr, &size);
    char* ip = inet_ntoa(addr.sin_addr);

    // Notify output window of new peer connection
    printf("[P2P]: Connected to peer at %hs\n", ip);

    while (1)
    {
        Packet* pkt = NetworkManager::getInstance().getPacket(_socket);

        switch (pkt->getProtocol())
        {
        case Packet::DISCONNECT:
            SendMessage(getRootWindow(), WM_EVENT_REMOVE_PEER, (WPARAM)this, 0);
            return;
        case Packet::STREAM_REQUEST:
            getStreamRequest(pkt);
            break;
        case Packet::STREAM_ALLOW:
            getStreamAllow(pkt);
            break;
        case Packet::STREAM_DENY:
            getStreamDeny(pkt);
            break;
        case Packet::STREAM_IMAGE:
            getStreamImage(pkt);
            break;
        case Packet::STREAM_CURSOR:
            getStreamCursor(pkt);
            break;
        case Packet::STREAM_CLOSE:
            getStreamClose();
            break;
        case Packet::CHAT_TEXT:
            getChatText(pkt);
            break;
        case Packet::CHAT_IS_TYPING:
            getChatIsTyping(pkt);
            break;
        case Packet::FILE_SEND_REQUEST:
            getFileSendRequest(pkt);
            break;
        case Packet::FILE_SEND_ALLOW:
            getFileSendAllow(pkt);
            break;
        case Packet::FILE_SEND_DENY:
            getFileSendDeny(pkt);
            break;
        case Packet::FILE_FRAGMENT:
            _fileSender->getFileFragment(pkt);
            break;
        case Packet::NAME:
            getName(pkt);
            break;
        }

        delete pkt;
    }
}

wchar_t* Peer::getName()
{
    return _name;
}

void Peer::getStreamClose()
{
    if (_streamSender) // Receiver closed stream window
    {
        _streamSender->stop();

        EnableMenuItem(_menu, 0, MF_ENABLED | MF_BYPOSITION);
        DrawMenuBar(_hWnd);
        EnableWindow(hChatStopSharingButton, FALSE);
    }
    else if (_hWnd_stream) // Sender halted stream
    {
        PostMessage(_hWnd_stream, WM_CLOSE, 0, 0);
    }
    if (_cursorUtil)
    {
        _cursorUtil->stop();
    }
}

//
// Only call with StreamSender object when threads are finished.
//
void Peer::clearStreamSender()
{
    delete _streamSender;
    _streamSender = NULL;
}

void Peer::clearCursorUtil()
{
    delete _cursorUtil;
    _cursorUtil = NULL;
}

void Peer::getName(Packet* pkt)
{
    _name = encode_utf16(pkt->getData());

    std::wstring str(L"Received peer name: ");
    str.append(_name);
    str.append(L"\n");
    OutputDebugString(str.c_str());

    // Update peer list
    updatePeerListBoxData();
}

void Peer::getChatIsTyping(Packet* pkt)
{
    // Show "Peer is typing..." label
    HWND isTypingLabel = GetDlgItem(_hWnd, IDC_PEER_CHAT_IS_TYPING_LABEL);
    ShowWindow(isTypingLabel, SW_SHOW);

    // Set timer to hide label after 1 second
    SetTimer(_hWnd, IDT_TIMER_PEER_IS_TYPING, 1000, (TIMERPROC)&hideChatIsTypingLabel);
}

void Peer::getChatText(Packet* pkt)
{
    // Hide "Peer is typing..." label
    HWND isTypingLabel = GetDlgItem(_hWnd, IDC_PEER_CHAT_IS_TYPING_LABEL);
    ShowWindow(isTypingLabel, SW_HIDE);

    // Kill timer, if exists
    KillTimer(_hWnd, IDT_TIMER_PEER_IS_TYPING);

    wchar_t *msg = encode_utf16(pkt->getData());

    // Append peer identifier to message
    std::wstring colon(L": ");
    std::wstring str = _name + colon + msg;

    addChat((LPWSTR)str.c_str());

    delete[] msg;
}

HWND Peer::getRoot()
{
    return _hWnd;
}

bool Peer::encode_utf8(std::pair<char*, size_t>* outPair, wchar_t* wstr)
{
    // Determine size needed for char array conversion
    size_t buffer_size;
    if (!wcstombs_s(&buffer_size, NULL, 0, wstr, _TRUNCATE))
    {
        // Convert wide char array to char array
        char* buffer = new char[buffer_size];
        if (!wcstombs_s(&buffer_size, buffer, buffer_size, wstr, _TRUNCATE))
        {
            outPair->first = buffer;
            outPair->second = buffer_size;
            return true;
        }        
    }
    return false;
}

wchar_t* Peer::encode_utf16(char* data)
{
    int szMsg = MultiByteToWideChar(CP_ACP, 0, data, -1, NULL, 0);
    wchar_t *buffer = new wchar_t[szMsg];
    MultiByteToWideChar(CP_ACP, 0, data, -1, (LPWSTR)buffer, szMsg);

    return buffer;
}

void Peer::sendChatMsg(wchar_t* msg)
{
    // Append peer identifier to message
    std::wstring wstr(getUserName());
    wstr.append(L": ");
    wstr.append(msg);

    addChat((LPWSTR)wstr.c_str());

    std::pair<char*, size_t> buffer;
    
    if (encode_utf8(&buffer, msg))
    {
        // Construct and send packet
        _worker->sendPacket(new Packet(Packet::CHAT_TEXT, buffer.first, buffer.second));
    }
    
    // Set focus to edit control again
    setInputFocus();
}

void Peer::DrawStreamImage(HDC hdc)
{
    RECT rect;
    CopyRect(&rect, &updateRegion);

    // Store client area for reference
    RECT window;
    GetClientRect(_hWnd_stream, &window);

    // Construct resized RECT that fits client area for reference
    int cx = _cachedStreamImage.GetWidth();
    int cy = _cachedStreamImage.GetHeight();
    float ratio = min((float)(window.right - window.left) / cx, (float)(window.bottom - window.top) / cy);
    CSize szSrc;
    szSrc.cx = (LONG)(cx * ratio);
    szSrc.cy = (LONG)(cy * ratio);
    CRect resized(CPoint(0, 0), szSrc);
    resized.OffsetRect(((window.right - window.left) - szSrc.cx) / 2, ((window.bottom - window.top) - szSrc.cy) / 2);

    // Set stretch mode to antialiased
    SetStretchBltMode(hdc, HALFTONE);

    // Determine if entire window is being redrawn
    if (onResize) // Entire screen needs to be redrawn
    {
        // Draw new stream image area
        if (!_cachedStreamImage.StretchBlt(hdc, resized))
        {
            printf("[DEBUG]: StretchBlt() failed on stream resize.\n");
        }
        onResize = false;
    }
    else // Only part of screen is being redrawn
    {
        // Grab parameters for RECT checking
        int width = _cachedStreamImage.GetWidth();
        int height = _cachedStreamImage.GetHeight();
        int szTile = _streamSender->getTileSize(width, height);

        // All tiles that intersect with updated region
        // must redraw, otherwise StretchBlt will create
        // visible artifacts.
        for (int x = 0; x < width; x += szTile)
        {
            for (int y = 0; y < height; y += szTile)
            {
                // Check if tile intersects updated region
                RECT tile, empty;
                tile.left = x;
                tile.top = y;
                tile.right = x + szTile;
                tile.bottom = y + szTile;
                
                if (IntersectRect(&empty, &tile, &rect))
                {
                    // Blit from original tile region
                    RECT original;
                    CopyRect(&original, &tile);

                    // Apply ratio and offset to tile
                    tile.left = (LONG)(tile.left * ratio);
                    tile.top = (LONG)(tile.top * ratio);
                    tile.right = (LONG)(tile.right * ratio);
                    tile.bottom = (LONG)(tile.bottom * ratio);
                    OffsetRect(&tile, resized.left, resized.top);

                    _cachedStreamImage.StretchBlt(
                        hdc,
                        tile,
                        original
                        );
                }
            }
        }
    }

    // Always fill non-streaming areas in window
    // Fill empty areas with background brush
    RECT fill;
    if (window.left < resized.left) // Fill left
    {
        fill.left = fill.top = 0;
        fill.bottom = window.bottom;
        fill.right = resized.left;
        FillRect(hdc, &fill, getDefaultBrush());
    }
    if (window.right > resized.right) // Fill right
    {
        fill.left = resized.right;
        fill.top = 0;
        fill.bottom = window.bottom;
        fill.right = window.right;
        FillRect(hdc, &fill, getDefaultBrush());
    }
    if (window.top < resized.top) // Fill top
    {
        fill.left = fill.top = 0;
        fill.right = window.right;
        fill.bottom = resized.top;
        FillRect(hdc, &fill, getDefaultBrush());
    }
    if (window.bottom > resized.bottom) // Fill bottom
    {
        fill.top = resized.bottom;
        fill.left = 0;
        fill.right = window.right;
        fill.bottom = window.bottom;
        FillRect(hdc, &fill, getDefaultBrush());
    }
}

void Peer::DrawStreamCursor(HDC hdc)
{
    // Create default cursor icon
    HICON cursor = (HICON)LoadImage(NULL, MAKEINTRESOURCE(IDC_ARROW), IMAGE_CURSOR, 0, 0, LR_SHARED);
    
    // Get icon bounds
    RECT bounds = _cursorUtil->getRect(cursor);
    
    // Store client area for reference
    RECT window;
    GetClientRect(_hWnd_stream, &window);

    // Construct resized RECT that fits client area for reference
    int cx = _cachedStreamImage.GetWidth();
    int cy = _cachedStreamImage.GetHeight();
    float ratio = min((float)(window.right - window.left) / cx, (float)(window.bottom - window.top) / cy);
    CSize szSrc;
    szSrc.cx = (LONG)(cx * ratio);
    szSrc.cy = (LONG)(cy * ratio);
    CRect resized(CPoint(0, 0), szSrc);
    resized.OffsetRect(((window.right - window.left) - szSrc.cx) / 2, ((window.bottom - window.top) - szSrc.cy) / 2);

    // Create icon rect
    RECT icon;
    icon.left = _cachedStreamCursor.x;
    icon.top = _cachedStreamCursor.y;
    icon.right = icon.left + (bounds.right - bounds.left);
    icon.bottom = icon.top + (bounds.bottom - bounds.top);

    // Apply ratio to icon rect
    icon.left = (LONG)(icon.left * ratio);
    icon.top = (LONG)(icon.top * ratio);
    icon.right = (LONG)(icon.right * ratio);
    icon.bottom = (LONG)(icon.bottom * ratio);

    // Offset rect to resized origin
    OffsetRect(&icon, resized.left, resized.top);

    // Draw cursor
    DrawIconEx(hdc, icon.left, icon.top, cursor, icon.right - icon.left, icon.bottom - icon.top, 0, NULL,
        DI_NORMAL | DI_COMPAT);
}

void Peer::getStreamImage(Packet* pkt)
{
    // Ignore packet if stream has been closed
    if (!_hWnd_stream)
    {
        return;
    }

    // Grab origin POINT preceding data
    POINT origin;
    size_t szPoint = sizeof(origin);

	// Not zero is error condition
	if (memcpy_s(&origin, sizeof(origin), pkt->getData(), sizeof(origin)))
	{
		return;
	}

	// Reconstruct CImage from remaining data
	IStream *pStream;
	if (CreateStreamOnHGlobal(0, TRUE, &pStream) == S_OK)
	{
		// Write bytes to stream
		if (IStream_Write(pStream, pkt->getData() + szPoint, pkt->getSize() - szPoint) == S_OK)
		{
			// Update region in cache
			CImage image;
			image.Load(pStream);
			HDC hdc = CreateCompatibleDC(GetDC(_hWnd_stream));
			SelectObject(hdc, _cachedStreamImage);
			image.BitBlt(hdc, origin.x, origin.y);
			ReleaseDC(_hWnd_stream, hdc);
			DeleteDC(hdc);

			// Invalidate with updated RECT
			int szTile = image.GetWidth();
			RECT rect;
			rect.left = origin.x;
			rect.top = origin.y;
			rect.right = origin.x + szTile;
			rect.bottom = origin.y + szTile;

            // Invalidate entire client area so I can draw anywhere
            // Save the actual region that changes to updateRegion
            CopyRect(&updateRegion, &rect);
            InvalidateRect(_hWnd_stream, NULL, FALSE);
			UpdateWindow(_hWnd_stream);

			image.Destroy();
		}
	}

	// Release memory
	pStream->Release();
}

void Peer::getStreamCursor(Packet * pkt)
{
    // Ignore packet if stream has been closed
    if (!_hWnd_stream)
    {
        return;
    }

    POINT pt = _cachedStreamCursor;

	// Not zero is error condition
	if (memcpy_s(&_cachedStreamCursor, sizeof(_cachedStreamCursor), pkt->getData(), sizeof(_cachedStreamCursor)))
	{
		return;
	}

    // Get size of icon
    HICON cursor = (HICON)LoadImage(NULL, MAKEINTRESOURCE(IDC_ARROW), IMAGE_CURSOR, 0, 0, LR_SHARED);
    RECT bounds = _cursorUtil->getRect(cursor);

    RECT rect;
    rect.left = min(pt.x, _cachedStreamCursor.x);
    rect.top = min(pt.y, _cachedStreamCursor.y);
    rect.right = max(pt.x + (bounds.right - bounds.left), _cachedStreamCursor.x);
    rect.bottom = max(pt.y + (bounds.bottom - bounds.top), _cachedStreamCursor.y);

    // Invalidate entire client area so I can draw anywhere
    // Save the actual region that changes to updateRegion
    CopyRect(&updateRegion, &rect);
    InvalidateRect(_hWnd_stream, NULL, FALSE);
    UpdateWindow(_hWnd_stream);
}

void Peer::getStreamRequest(Packet* pkt)
{
    StreamSender::StreamInfo info;
    
    // Not zero is error condition
    if (memcpy_s(&info, sizeof(info), pkt->getData(), sizeof(info)))
    {
        return;
    }

    // Pop up window to accept or deny stream request
    if (displayStreamRequestMessageBox(info.name) == IDYES)
    {
        // Create stream window
        openStreamWindow();

        // Update stream window title
        std::wstring str(_name);
        str.append(L" - ");
        str.append(info.name);
        SetWindowText(_hWnd_stream, str.c_str());

        // Set streaming window size
        RECT rect;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
        if (rect.right < info.width || rect.bottom < info.height) // Image larger than desktop
        {
            MoveWindow(_hWnd_stream, 0, 0, rect.right, rect.bottom, TRUE);
        }
        else // Image smaller than desktop
        {
            RECT img;
            img.left = img.top = 0;
            img.right = info.width;
            img.bottom = info.height;

            // Center window
            centerWindow(_hWnd_stream, img);
        }

        // Initialize cached image
        _cachedStreamImage.Create(info.width, info.height, info.bpp);

        _worker->sendPacket(new Packet(Packet::STREAM_ALLOW));
    }
    else
    {
        _worker->sendPacket(new Packet(Packet::STREAM_DENY));
    }
}

void Peer::getStreamAllow(Packet* pkt)
{
    EnableWindow(hChatStopSharingButton, true);

    _cursorUtil = new CursorUtil(this, _hWnd_stream_src);
    _cursorUtil->stream(30);

    _streamSender->stream(_hWnd_stream_src);
}

void Peer::getStreamDeny(Packet* pkt)
{
    EnableMenuItem(_menu, 0, MF_ENABLED | MF_BYPOSITION);
    DrawMenuBar(_hWnd);
}

void Peer::flushShareMenu()
{
    // Remove old menu items
    int idx = GetMenuItemCount(_menuShareScreen);
    while (--idx >= 0)
    {
        DeleteMenu(_menuShareScreen, idx, MF_BYPOSITION);
    }
}

void Peer::createMenu(HWND hWnd)
{
    _menu = CreateMenu();
    _menuShareScreen = CreatePopupMenu();
    
    //AppendMenu(_menuShareScreen, MF_STRING, 0, L"Desktop");
    AppendMenu(_menu, MF_STRING | MF_POPUP, (UINT)_menuShareScreen, L"Share Screen");

    SetMenu(hWnd, _menu);

    MENUINFO mi;
    memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.fMask = MIM_STYLE;
    mi.dwStyle = MNS_NOTIFYBYPOS;
    SetMenuInfo(_menu, &mi);
}

void Peer::onShareMenuInit()
{
    size_t maxTitleLen = 80;

    // Add new menu items and populate HWND list
    WindowUtil wndUtil;
    auto hwnds = wndUtil.getOpenWindows();
    int sz = hwnds.size();
    for (int idx = 0; idx < sz; ++idx)
    {
        // Grab window title and truncate it with ellipsis
        std::wstring title = wndUtil.getWindowTitle(hwnds.at(idx));
        if (title.length() > maxTitleLen)
        {
            title = title.substr(0, maxTitleLen);
            title.append(L"...");
        }

        // Get program icon
        HICON hIcon = (HICON) SendMessage(hwnds.at(idx), WM_GETICON, 1, 0);
        if (!hIcon)
        {
            hIcon = (HICON) GetClassLong(hwnds.at(idx), GCL_HICON);
        }

        HBITMAP hBitmap;
        ICONINFO iconinfo;
        GetIconInfo(hIcon, &iconinfo);
        ImageUtil util;
        hBitmap = util.makeBitMapTransparent(iconinfo.hbmColor);

        MENUITEMINFO menuInfo = { 0 };
        menuInfo.cbSize = sizeof(MENUITEMINFO);
        menuInfo.fMask = MIIM_STRING | MIIM_DATA | MIIM_BITMAP;
        menuInfo.dwItemData = (ULONG_PTR) hwnds.at(idx);
        menuInfo.dwTypeData = (LPWSTR) title.c_str();
        menuInfo.hbmpItem = hBitmap;

        InsertMenuItem(_menuShareScreen, 0, FALSE, &menuInfo);
    }

    // Add separator
    MENUITEMINFO mi = { 0 };
    mi.cbSize = sizeof(MENUITEMINFO);
    mi.fMask = MIIM_FTYPE;
    mi.fType = MFT_SEPARATOR;
    InsertMenuItem(_menuShareScreen, 0, FALSE, &mi);

    // Add desktop
    MENUITEMINFO mi2 = { 0 };
    mi2.cbSize = sizeof(MENUITEMINFO);
    mi2.fMask = MIIM_STRING | MIIM_DATA | MIIM_BITMAP;
    mi2.dwItemData = (ULONG_PTR)GetDesktopWindow();
    mi2.dwTypeData = L"Desktop";
    InsertMenuItem(_menuShareScreen, 0, FALSE, &mi2);
}

HMENU Peer::getMenu()
{
    return _menu;
}

HMENU Peer::getShareMenu()
{
    return _menuShareScreen;
}