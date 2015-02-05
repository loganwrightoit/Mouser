#include "stdafx.h"
#include "Peer.h"
#include <thread>
#include <string>
#include "Lmcons.h" // UNLEN

Peer::Peer(SOCKET peer_socket = 0)
: _socket(peer_socket), _cursor(POINT{ 0, 0 }), _hWnd(0), _hWnd_stream(0)
{
    _name = L"Unknown";

    // Start receive thread
    std::thread t(&Peer::rcvThread, this);
    t.detach();
}

Peer::~Peer()
{
    shutdown(_socket, SD_BOTH);
    closesocket(_socket);

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

void Peer::setChatWindow(HWND hWnd)
{
    _hWnd = hWnd;
}

void Peer::clearHwnd()
{
    _hWnd = 0;
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
        HWND root = getRootWindow();
        if (SendMessage(root, WM_EVENT_OPEN_PEER_CHAT, (WPARAM)this, NULL))
        {
            setInputFocus();
        }
    }
}

void Peer::setInputFocus()
{
    HWND editCtrl = GetDlgItem(_hWnd, IDC_PEER_CHAT_EDITBOX);
    SetFocus(editCtrl);
    SendDlgItemMessage(editCtrl, IDC_PEER_CHAT_EDITBOX, EM_SETSEL, 0, -1);
}

void Peer::openStreamWindow()
{
    //_hWnd_stream = getWindow(WindowType::StreamWin);
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

wchar_t* Peer::getUserName()
{
    // Send user name to peer
    wchar_t name[UNLEN + 1];
    DWORD szName = sizeof(name);
    if (GetUserName(name, &szName))
    {
        return name;
    }

    return L"Unknown";
}

void Peer::sendName()
{
    std::pair<char*, size_t> buffer = encode_utf8(getUserName());

    // Send name to peer
    Packet* pkt = new Packet(Packet::NAME, buffer.first, buffer.second);
    NetworkManager::getInstance().sendPacket(_socket, pkt);
    delete pkt;
}

void Peer::rcvThread()
{
    // Send peer name
    sendName();

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
        if (NetworkManager::getInstance().isSocketReady(_socket)) // Seems to be CPU intensive
        {
            Packet * pkt = nullptr;

            try
            {
                pkt = NetworkManager::getInstance().getPacket(_socket);

                switch (pkt->getProtocol())
                {
                case Packet::DISCONNECT:
                    PeerHandler::getInstance().disconnectPeer(this);
                    return;
                case Packet::STREAM_BEGIN:
                    getStreamOpen(pkt);
                    break;
                case Packet::STREAM_END:
                    getStreamClose(pkt);
                    break;
                case Packet::STREAM_IMAGE:
                    getStreamImage(pkt);
                    break;
                case Packet::STREAM_CURSOR:
                    getStreamCursor(pkt);
                    break;
                case Packet::CHAT_TEXT:
                    getChatText(pkt);
                    break;
                case Packet::CHAT_IS_TYPING:
                    getChatIsTyping(pkt);
                    break;
                case Packet::NAME:
                    getName(pkt);
                    break;
                }
            }
            catch (std::exception) {}

            if (pkt != nullptr)
            {
                delete pkt;
            }
            else
            {
                PeerHandler::getInstance().disconnectPeer(this);
                return;
            }
        }
    }
}

wchar_t* Peer::getName()
{
    return _name;
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

HWND Peer::getRoot()
{
    return _hWnd;
}

void Peer::getStreamOpen(Packet* pkt)
{

}

void Peer::getStreamClose(Packet* pkt)
{

}

std::pair<char*, size_t> Peer::encode_utf8(wchar_t* wstr)
{
    // Determine size needed for char array conversion
    size_t buffer_size;
    wcstombs_s(&buffer_size, NULL, 0, wstr, _TRUNCATE);

    // Convert wide char array to char array
    char* buffer = new char[buffer_size];
    wcstombs_s(&buffer_size, buffer, buffer_size, wstr, _TRUNCATE);

    return std::make_pair(buffer, buffer_size);
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

    std::pair<char*, size_t> buffer = encode_utf8(msg);

    // Construct and send packet
    Packet* pkt = new Packet(Packet::CHAT_TEXT, buffer.first, buffer.second);
    NetworkManager::getInstance().sendPacket(_socket, pkt);

    // Set focus to edit control again
    setInputFocus();

    delete pkt;
}

void Peer::sendStreamImage()
{

}

void Peer::getStreamImage(Packet* pkt)
{
    /*
    IStream *pStream;
    HRESULT result = CreateStreamOnHGlobal(0, TRUE, &pStream);
    IStream_Write(pStream, buffer, length);
    // Create image from stream
    CImage image;
    image.Load(pStream);
    // Draw to window
    HDC hdc = GetDC(hStreamWindow);
    image.BitBlt(hdc, 0, 0);
    ReleaseDC(hStreamWindow, hdc);
    pStream->Release();
    */
}

void Peer::sendStreamCursor()
{
    POINT p;
    GetCursorPos(&p);
    HWND hWnd = GetDesktopWindow();
    if (ScreenToClient(hWnd, &p))
    {
        // Only send update if cursor location changed
        if (_cursor.x != p.x || _cursor.y != p.y)
        {
            _cursor.x = p.x;
            _cursor.y = p.y;

            // Convert struct to char array
            char * data = new char[sizeof(_cursor)];
            std::memcpy(data, &_cursor, sizeof(_cursor));

            // Construct and send packet
            Packet * pkt = new Packet(Packet::STREAM_CURSOR, data, sizeof(_cursor));
            NetworkManager::getInstance().sendPacket(_socket, pkt);

            delete pkt;
        }
    }
}

void Peer::getStreamCursor(Packet * pkt)
{
    std::memcpy(&_cursor, pkt->getData(), sizeof(_cursor));

    wchar_t buffer[256];
    swprintf(buffer, 256, L"[P2P]: Received cursor location: %d, %d", _cursor.x, _cursor.y);
    AddOutputMsg(buffer);

    // Draw static cursor on screen after blit
    // Always get last location from a cache in case update has not occurred
    HICON NormalCursor = (HICON)LoadImage(NULL, MAKEINTRESOURCE(IDC_ARROW), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    HDC hDC = GetDC(_hWnd_stream);
    DrawIconEx(hDC, _cursor.x, _cursor.y, NormalCursor, 0, 0, NULL, NULL, DI_DEFAULTSIZE | DI_NORMAL);
}