#include "stdafx.h"
#include "Peer.h"
#include <thread>
#include <string>
#include "math.h"
#include <Shlobj.h>

#define NOMINMAX

Peer::Peer(SOCKET peer_socket = 0)
: _socket(peer_socket), _hWnd(0), _hWnd_stream(0), _streamSender(0), _cursorUtil(0)
{
    _name = L"";

    // Create queue mutex lock
    if ((ghMutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
    {
        AddOutputMsg(L"[P2P]: CreateMutex() failed, queue not thread-safe.");
    }

    // Send peer name
    sendName();

    // Start send thread
    std::thread st(&Peer::sendThread, this);
    st.detach();

    // Start receive thread
    std::thread rt(&Peer::rcvThread, this);
    rt.detach();
}

Peer::~Peer()
{
    // Stop peer cursor and image streams
    _streamSender->stop();
    _cursorUtil->stop();

    shutdown(_socket, SD_BOTH);
    closesocket(_socket);
    CloseHandle(ghMutex);

    if (_hWnd)
    {
        PostMessage(_hWnd, WM_CLOSE, 0, 0);
    }
    if (_hWnd_stream)
    {
        PostMessage(_hWnd_stream, WM_CLOSE, 0, 0);
    }
    if (_name)
    {
        delete[] _name;
    }
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

void Peer::onDestroyRoot()
{
    _hWnd = _hWnd_stream = _hWnd_stream_src = 0;

    if (_streamSender)
    {
        _streamSender->stop();
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

            // Focus on input text area
            setInputFocus();
        }
    }
}

void Peer::sendPacket(Packet* pkt)
{
    SendMessage(getRootWindow(), WM_EVENT_SEND_PACKET, (WPARAM)&std::make_pair(this, pkt), NULL);
}

//
// Single-threaded use only by main thread!
// Can stall program waiting for queue
//
void Peer::queuePacket(Packet* pkt)
{
    if (WaitForSingleObject(ghMutex, INFINITE) == WAIT_OBJECT_0)
    {
        sendQueue.push(pkt);
        ReleaseMutex(ghMutex);
    }
}

void Peer::sendThread()
{
    while (1)
    {
        if (sendQueue.size() > 0)
        {
            if (WaitForSingleObject(ghMutex, INFINITE) == WAIT_OBJECT_0)
            {
                Packet* pkt = sendQueue.front();
                NetworkManager::getInstance().sendPacket(_socket, pkt);
                sendQueue.pop();
                delete pkt;
                ReleaseMutex(ghMutex);
            }
        }
        else
        {
            Sleep(15); // Awakened 66 times a second, more than sufficient
        }
    }
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
        HWND root = getRootWindow();
        SendMessage(root, WM_EVENT_OPEN_PEER_STREAM, (WPARAM)this, NULL);
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
        sendPacket(new Packet(Packet::NAME, buffer.first, buffer.second));
    }
}

void Peer::makeFileSendRequest(wchar_t* path)
{
    // Open file and seek to end (ate) to get size
    std::ifstream file(path, std::ifstream::binary | std::ifstream::in | std::ifstream::ate);

    if (file.is_open())
    {
        // Send file info to peer (filename, size)
        wcscpy_s(_file.path, wcslen(path) + 1, path);
        _file.size = (size_t)file.tellg();
        char* data = new char[sizeof(_file)];
        memcpy_s(data, sizeof(_file), &_file, sizeof(_file));
        sendPacket(new Packet(Packet::FILE_SEND_REQUEST, data, sizeof(_file)));

        // Close file
        file.close();
    }
}

void Peer::makeStreamRequest(HWND hWnd)
{
    if (_streamSender)
    {
        addChat(L"--> Stopped sharing screen.");
        _streamSender->stop();
        return;
    }
    else
    {
        addChat(L"--> Sent screen share request, awaiting response.");
        EnableWindow(hChatStreamButton, false);
    }

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
        AddOutputMsg(L"[DEBUG]: StreamInfo memcpy_s failed, aborting.");
        delete[] data;
        return;
    }

    sendPacket(new Packet(Packet::STREAM_REQUEST, data, sizeof(info)));
}

void Peer::doFileSendThread()
{
    // Open file
    std::ifstream file(_file.path, std::ifstream::binary);

    if (file.is_open())
    {
        _remainingFile = _file.size;

        // Send file fragments to peer
        while (1)
        {
            if (getQueueSize() < 4)
            {
                char buffer[FILE_BUFFER];
                size_t size;
                if (file.read(buffer, sizeof(buffer)))
                {
                    _remainingFile -= FILE_BUFFER;

                    // Send fragment
                    char* data = new char[sizeof(buffer)];
                    memcpy_s(data, sizeof(buffer), buffer, sizeof(buffer));
                    sendPacket(new Packet(Packet::FILE_FRAGMENT, data, sizeof(buffer)));
                }
                else if ((size = (size_t)file.gcount()) > 0)
                {
                    _remainingFile -= size;

                    // Send final fragment
                    char* data = new char[size];
                    memcpy_s(data, size, buffer, size); // Buffer only contains data up to file.gcount()
                    sendPacket(new Packet(Packet::FILE_FRAGMENT, data, size));
                }
                else
                {
                    // Notify peer chat of completion
                    wchar_t buffer[256];
                    PathStripPath(_file.path);
                    swprintf(buffer, 256, L"--> File %ls (%d bytes) sent to peer.", _file.path, _file.size);
                    addChat(buffer);

                    SetWindowText(_hWnd, _name);

                    file.close();
                    return;
                }

                // Update percentage label in peer window
                wchar_t buffer2[256];
                swprintf(buffer2, 256, L"%ls - Sending file: %.0f%%", _name, (1 - (float)_remainingFile / _file.size) * 100);
                SetWindowText(_hWnd, buffer2);
            }
        }
    }
}

int Peer::displayFileSendRequestMessageBox()
{
    wchar_t fileName[MAX_PATH];
    wcscpy_s(fileName, MAX_PATH, _file.path);
    PathStripPath(fileName);

    wchar_t buffer[256];
    swprintf(buffer, 256, L"%ls wants to send you a file:\n\n%ls (%i bytes)\n\nDo you want to accept it?", _name, fileName, _file.size);

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
    swprintf(buffer, 256, L"%ls wants to share a screen with you: %ls.\nDo you want to accept it?", _name, name);

    int msgboxID = MessageBox(
        NULL,
        buffer,
        L"Accept Screen Share",
        MB_ICONEXCLAMATION | MB_YESNO
        );

    return msgboxID;
}

void Peer::getFileSendRequest(Packet* pkt)
{
    // Save file info
    // Not zero is error condition
    if (memcpy_s(&_file, sizeof(_file), pkt->getData(), sizeof(_file)))
    {
        return;
    }

    if (displayFileSendRequestMessageBox() == IDYES)
    {
        // Create file on desktop
        if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_CREATE, NULL, 0, _tempPath)))
        {
            // Grab some path properties
            wchar_t fileName[MAX_PATH];
            _wsplitpath_s(_file.path, NULL, 0, NULL, 0, fileName, MAX_PATH, _tempExt, MAX_PATH);

            // Build a new path with temporary file extension
            PathAppend(_tempPath, fileName);
            PathRenameExtension(_tempPath, L".tmp");

            // Save remaining size for decrementing as fragments received
            _remainingFile = _file.size;

            // Open file in preparation for file fragments
            _outFile.open(_tempPath, std::ifstream::binary);
            if (_outFile.is_open())
            {
                // Send accept packet to begin receiving file
                sendPacket(new Packet(Packet::FILE_SEND_ALLOW));
            }
            else
            {
                AddOutputMsg(L"[P2P]: Unable to open temporary file for receiving send.");
            }
        }
        else
        {
            AddOutputMsg(L"[P2P]: Unable to get temporary file path.");
        }
    }
    else
    {
        sendPacket(new Packet(Packet::FILE_SEND_DENY));
        addChat(L"--> Denied file send request.");
    }
}

void Peer::getFileSendAllow(Packet* pkt)
{
    addChat(L"--> Peer accepted file send request, beginning transfer.");

    // Start new thread to send file
    std::thread t(&Peer::doFileSendThread, this);
    t.detach();
}

void Peer::getFileSendDeny(Packet* pkt)
{
    addChat(L"--> Peer denied file send request.");
}

void Peer::getFileFragment(Packet* pkt)
{
    // Check that file is open
    if (_outFile.is_open())
    {
        _outFile.write(pkt->getData(), pkt->getSize());

        // Decrement expected size
        _remainingFile -= pkt->getSize();

        // Close file if expected reaches 0
        if (_remainingFile <= 0)
        {
            _outFile.close();

            // Rename extension of temporary file
            wchar_t oldPath[MAX_PATH];
            wcscpy_s(oldPath, MAX_PATH, _tempPath);
            PathRenameExtension(_tempPath, _tempExt);
            _wrename(oldPath, _tempPath);

            // Notify peer chat of completion
            wchar_t buffer[256];
            PathStripPath(_file.path);
            swprintf(buffer, 256, L"--> File %ls (%d bytes) saved to desktop.", _file.path, _file.size);
            addChat(buffer);

            SetWindowText(_hWnd, _name);
            return;
        }

        // Update percentage label in peer window
        wchar_t buffer[256];
        swprintf(buffer, 256, L"%ls - Receiving file: %.0f%%", getUserName(), (1 - (float) _remainingFile / _file.size) * 100);
        openChatWindow();
        SetWindowText(_hWnd, buffer);
    }
}

void Peer::rcvThread()
{
    // Gather peer info
    sockaddr_in addr;
    int size = sizeof(addr);
    getpeername(_socket, (sockaddr*)&addr, &size);
    char* ip = inet_ntoa(addr.sin_addr);

    // Notify output window of new peer connection
    wchar_t buffer[256];
    swprintf(buffer, 256, L"[P2P]: Connected to peer at %hs", ip);
    AddOutputMsg(buffer);

    while (1)
    {
        Packet* pkt = NetworkManager::getInstance().getPacket(_socket);

        switch (pkt->getProtocol())
        {
        case Packet::DISCONNECT:
            PeerHandler::getInstance().disconnectPeer(this);
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
            getFileFragment(pkt);
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
        addChat(L"--> Peer closed screen share.");
    }
    else if (_hWnd_stream) // Sender halted stream
    {
        PostMessage(_hWnd_stream, WM_CLOSE, 0, 0);
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

size_t Peer::getQueueSize() const
{
    return sendQueue.size();
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
        sendPacket(new Packet(Packet::CHAT_TEXT, buffer.first, buffer.second));
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
            AddOutputMsg(L"[DEBUG]: StretchBlt() failed on stream resize.");
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

    /*
    BOOL WINAPI DrawIconEx(
        _In_      HDC hdc,
        _In_      int xLeft,
        _In_      int yTop,
        _In_      HICON hIcon,
        _In_      int cxWidth,
        _In_      int cyWidth,
        _In_      UINT istepIfAniCur,
        _In_opt_  HBRUSH hbrFlickerFreeDraw,
        _In_      UINT diFlags
        );
    */
    DrawIconEx(hdc, icon.left, icon.top, cursor, icon.right - icon.left, icon.bottom - icon.top, 0, NULL,
        DI_NORMAL | DI_COMPAT);



    //DrawIcon(hdc, _cachedStreamCursor.x, _cachedStreamCursor.y, NormalCursor);
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

        sendPacket(new Packet(Packet::STREAM_ALLOW));
    }
    else
    {
        sendPacket(new Packet(Packet::STREAM_DENY));
    }
}

void Peer::getStreamAllow(Packet* pkt)
{
    addChat(L"--> Share request accepted, you are now sharing your screen.");
    EnableWindow(hChatStreamButton, true);

    _cursorUtil = new CursorUtil(this, _hWnd_stream_src);
    _cursorUtil->stream(30);

    _streamSender->stream(_hWnd_stream_src);
}

void Peer::getStreamDeny(Packet* pkt)
{
    addChat(L"--> Share request denied.");
    EnableWindow(hChatStreamButton, true);
}