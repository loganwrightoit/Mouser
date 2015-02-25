#include "stdafx.h"
#include "Peer.h"
#include <thread>
#include <string>
#include "math.h"
#include <Shlobj.h>

#define NOMINMAX

Peer::Peer(SOCKET peer_socket = 0)
: _socket(peer_socket), _hWnd(0), _hWnd_stream(0), _streamSender(0), _cursorUtil(0), _streaming(false)
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
    shutdown(_socket, SD_BOTH);
    closesocket(_socket);

    // Stop peer cursor and image streams
    _streamSender->stop();
    _cursorUtil->stop();

    // Clear queue
    //if (!sendQueue.empty())
    //{
    //    auto iter = sendQueue.front
    //}

    CloseHandle(ghMutex);

    if (_hWnd)
    {
        DestroyWindow(_hWnd);
    }
    if (_hWnd_stream)
    {
        DestroyWindow(_hWnd_stream);
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
    _hWnd = 0;

    if (_streamSender != nullptr)
    {
        _streamSender->stop();
        _streamSender->~StreamSender();
    }

    _hWnd_stream = 0;
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

void Peer::streamTo()
{
    if (!_streaming)
    {
        _streaming = true;
        HWND hWnd = GetDesktopWindow();

        _cursorUtil = new CursorUtil(this, hWnd);
        _cursorUtil->stream(60);

        _streamSender = new StreamSender(this, _hWnd_stream);
        _streamSender->stream(hWnd);
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

void Peer::AddChat(LPWSTR msg)
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

void Peer::doFileSendRequest(wchar_t* path)
{
    // Open file and seek to end (ate) to get size
    std::ifstream file(path, std::ifstream::binary | std::ifstream::in | std::ifstream::ate);

    if (file.is_open())
    {
        OutputDebugString(L"[DEBUG]: Opened file for sending, preparing info packet\n");

        size_t szFile = (size_t)file.tellg();
        file.seekg(0, std::ifstream::beg); // Seek back to beginning

        OutputDebugString(L"[DEBUG]: seeked to beginning of file stream in path: ");
        OutputDebugString(path);
        OutputDebugString(L"\n");

        // Send file info to peer (filename, size)
        wcscpy_s(_file.path, wcslen(path) + 1, path);
        OutputDebugString(L"[DEBUG]: reached 1.\n");
        _file.size = szFile;
        OutputDebugString(L"[DEBUG]: reached 2.\n");
        char* data = new char[sizeof(_file)];
        OutputDebugString(L"[DEBUG]: reached 3.\n");
        memcpy_s(data, sizeof(_file), &_file, sizeof(_file));
        OutputDebugString(L"[DEBUG]: reached 4.\n");
        sendPacket(new Packet(Packet::FILE_SEND_REQUEST, data, sizeof(_file)));

        OutputDebugString(L"[DEBUG]: sent info packet.\n");

        file.close();
    }
}

void Peer::doFileSendThread()
{
    // Open file and seek to end (ate) to get size
    std::ifstream file(_file.path, std::ifstream::binary);

    if (file.is_open())
    {
        // Send file fragments to peer
        while (1)
        {
            if (getQueueSize() < 4)
            {
                OutputDebugString(L"[DEBUG]: Preparing packet.\n");

                char buffer[DEFAULT_BUFFER_SIZE];
                size_t size;
                if (file.read(buffer, sizeof(buffer)))
                {
                    OutputDebugString(L"[DEBUG]: Sending file fragment packet.\n");

                    char* data = new char[sizeof(buffer)];
                    memcpy_s(data, sizeof(data), buffer, sizeof(buffer));
                    sendPacket(new Packet(Packet::FILE_FRAGMENT, data, sizeof(buffer)));
                }
                else if ((size = (size_t)file.gcount()) > 0)
                {
                    wchar_t buffer1[256];
                    swprintf(buffer1, 256, L"Sending fragment: %hs", buffer);
                    AddOutputMsg(buffer1);

                    // Send final fragment
                    char* data = new char[size];
                    memcpy_s(data, size, buffer, size); // Restrain buffer to sizeof(data)

                    sendPacket(new Packet(Packet::FILE_FRAGMENT, data, size));
                }
                else
                {
                    OutputDebugString(L"[P2P]: File send finished.");
                    file.close();
                    return;
                }
            }
        }
    }
}

int Peer::DisplayAcceptFileSendMessageBox()
{
    wchar_t fileName[MAX_PATH];
    wcscpy_s(fileName, MAX_PATH, _file.path);
    PathStripPath(fileName);

    std::wstring str;
    str.append(getName());
    str.append(L" wants to send you a file: ");
    str.append(fileName);
    str.append(L" (");
    str.append(std::to_wstring(_file.size));
    str.append(L" bytes)\n\n");
    str.append(L"Do you want to accept it?");

    int msgboxID = MessageBox(
        NULL,
        str.c_str(),
        L"Accept File Send",
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

    if (DisplayAcceptFileSendMessageBox() == IDYES)
    {
        // Create file on desktop
        wchar_t szPath[MAX_PATH];

        if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_CREATE, NULL, 0, szPath)))
        {
            PathAppend(szPath, TEXT("mouser_file.tmp"));

            HANDLE newFile = CreateFile(
                szPath,                 // name of the write
                GENERIC_WRITE,          // open for writing
                0,                      // do not share
                NULL,                   // default security
                CREATE_ALWAYS,          // create new file
                FILE_ATTRIBUTE_NORMAL,  // normal file
                NULL);                  // no attr. template

            if (newFile == INVALID_HANDLE_VALUE)
            {
                AddOutputMsg(L"[P2P]: Unable to create temporary file.");
                sendPacket(new Packet(Packet::FILE_SEND_DENY));
            }

            // Save temporary path for renaming once file send is complete
            wcscpy_s(_tempPath, sizeof(_file.path) + 1, szPath);

            // Save remaining size for decrementing as fragments received
            _remainingFile = _file.size;

            // Open file in preparation for file fragments
            _outFile.open(_file.path, std::ifstream::binary | std::ifstream::out);
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
        AddOutputMsg(L"[P2P]: Denied file send request.");
    }
}

void Peer::getFileSendAllow(Packet* pkt)
{
    AddOutputMsg(L"[P2P]: Peer accepted file send request.");

    // Start new thread to send file
    std::thread t(&Peer::doFileSendThread, this);
    t.detach();
}

void Peer::getFileSendDeny(Packet* pkt)
{
    AddOutputMsg(L"[P2P]: Peer denied file send request.");
}

void Peer::getFileFragment(Packet* pkt)
{
    AddOutputMsg(L"[DEBUG]: Received file fragment.");

    // Check that file is open
    if (_outFile.is_open())
    {
        wchar_t buffer1[256];
        swprintf(buffer1, 256, L"Received fragment %hs", pkt->getData());
        AddOutputMsg(buffer1);

        _outFile.write(pkt->getData(), pkt->getSize());

        // Decrement expected size
        _remainingFile -= pkt->getSize();

        // Close file if expected reaches 0
        if (_remainingFile <= 0)
        {
            _outFile.close();

            // Rename temporary file
            wchar_t fileName[MAX_PATH];
            wcscpy_s(fileName, MAX_PATH, _file.path);
            PathStripPath(fileName);
            wchar_t newPath[MAX_PATH];
            wcscpy_s(newPath, MAX_PATH, _tempPath);
            PathRemoveFileSpec(newPath);
            PathAddBackslash(newPath);
            PathAppend(newPath, fileName);
            _wrename(_tempPath, newPath);

            // Change file send label to "Received File."
            AddOutputMsg(L"Received file.");
            return;
        }

        // Update percentage label in peer window
        wchar_t buffer[256];
        swprintf(buffer, 256, L"Receiving file (%i)", _remainingFile / _file.size);
        AddOutputMsg(buffer);
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
        case Packet::STREAM_INFO:
            getStreamInfo(pkt);
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
    DestroyWindow(_hWnd_stream);
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

    AddChat((LPWSTR)str.c_str());

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

    AddChat((LPWSTR)wstr.c_str());

    std::pair<char*, size_t> buffer;
    
    if (encode_utf8(&buffer, msg))
    {
        // Construct and send packet
        sendPacket(new Packet(Packet::CHAT_TEXT, buffer.first, buffer.second));
    }
    
    // Set focus to edit control again
    setInputFocus();
}

void Peer::DrawStreamImage(HDC hdc, RECT rect)
{
    /*
    // Image size determined by window width and height
    RECT dest;
    GetWindowRect(_hWnd_stream, &dest);

    int img_cx = image.GetWidth();
    int img_cy = image.GetHeight();

    float ratio = min((float)(dest.right - dest.left) / img_cx, (float)(dest.bottom - dest.top) / img_cy);

    // Get image size
    CSize szSrc;
    szSrc.cx = (LONG)(img_cx * ratio);
    szSrc.cy = (LONG)(img_cy * ratio);

    // Get RECT for drawing
    CRect src(CPoint(0, 0), szSrc);

    // Center the RECT
    src.OffsetRect(((dest.right - dest.left) - szSrc.cx) / 2, ((dest.bottom - dest.top) - szSrc.cy) / 2);
    */
    
    // TODO: Work on resizing for the tile-based system

    //SetStretchBltMode(hdc, HALFTONE);
    if (!_cachedStreamImage.BitBlt(hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, rect.left, rect.top))
    {
        AddOutputMsg(L"[DEBUG]: BitBlt failed on read.");
    }
}

void Peer::DrawStreamCursor(HDC hdc, RECT rect)
{
    // TODO: Need to handle resizing once implemented
    // TODO: Try to approximate exact size of cursor for blitting

    HICON NormalCursor = (HICON)LoadImage(NULL, MAKEINTRESOURCE(IDC_ARROW), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    DrawIcon(hdc, _cachedStreamCursor.x, _cachedStreamCursor.y, NormalCursor);
}

void Peer::getStreamImage(Packet* pkt)
{
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
			InvalidateRect(_hWnd_stream, &rect, FALSE);
			UpdateWindow(_hWnd_stream);

			image.Destroy();
		}
	}

	// Release memory
	pStream->Release();
}

void Peer::getStreamCursor(Packet * pkt)
{
    POINT pt = _cachedStreamCursor;

	// Not zero is error condition
	if (memcpy_s(&_cachedStreamCursor, sizeof(_cachedStreamCursor), pkt->getData(), sizeof(_cachedStreamCursor)))
	{
		return;
	}

    RECT rect;
    rect.left = min(pt.x, _cachedStreamCursor.x);
    rect.top = min(pt.y, _cachedStreamCursor.y);
    rect.right = max(pt.x + 30, _cachedStreamCursor.x);
    rect.bottom = max(pt.y + 30, _cachedStreamCursor.y);

    InvalidateRect(_hWnd_stream, &rect, FALSE);
    UpdateWindow(_hWnd_stream);
}

void Peer::getStreamInfo(Packet* pkt)
{
    // Create stream window
    openStreamWindow();

    // Populate struct from packet
    StreamSender::StreamInfo info;

	// Not zero is error condition
	if (memcpy_s(&info, sizeof(info), pkt->getData(), sizeof(info)))
	{
		return;
	}

    // Update stream window title
    std::wstring str(_name);
    str.append(L" - ");
    str.append(info.name);
    SetWindowText(_hWnd_stream, str.c_str());

    // Resize and center stream window
    RECT rect;
    rect.left = rect.top = 0;
    rect.right = info.width;
    rect.bottom = info.height;
    centerWindow(_hWnd_stream, rect);

    // Initialize cached image
    _cachedStreamImage.Create(info.width, info.height, info.bpp);
}