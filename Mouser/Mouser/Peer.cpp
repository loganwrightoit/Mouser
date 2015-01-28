#include "stdafx.h"
#include "Peer.h"
#include <thread>

Peer::Peer(SOCKET peer_socket = 0)
: _socket(peer_socket), _cursor(POINT{ 0, 0 })
{
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
        //DestroyWindow(_hWnd);
    }
    if (_hWnd_stream)
    {
        //DestroyWindow(_hWnd_stream);
    }
}

void Peer::openChatWindow()
{
    // Create peer window
    _hWnd = getWindow(WindowType::PeerWin);

    // Add class pointer to window handle for message processing
    SetWindowLongPtr(_hWnd, GWLP_USERDATA, (LONG_PTR)this);

    // Create controls in processor
    /*
    // Create output edit box
    hChatEditBox = CreateWindowEx(WS_EX_CLIENTEDGE,
        L"LISTBOX",
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL,
        0,
        0,
        300,
        300,
        _hWnd,
        (HMENU)IDC_PEER_CHAT_LISTBOX,
        GetModuleHandle(NULL),
        NULL);

    // Create send data button
    _hWnd_sendData = CreateWindowEx(NULL,
        L"BUTTON",
        L"Send Data",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        0, // x padding
        300, // y padding
        300, // width
        30,  // height
        _hWnd,
        NULL,
        GetModuleHandle(NULL),
        NULL);
    */
}

void Peer::openStreamWindow()
{
    _hWnd_stream = getWindow(WindowType::StreamWin);
}

SOCKET Peer::getSocket() const
{
    return _socket;
}

void Peer::AddChat(LPWSTR msg)
{
    //SendMessage(hPeerChatListBox, LB_ADDSTRING, 0, (LPARAM)msg);
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

    /*
    const size_t cSize = strlen(ip) + 1;
    std::wstring wc(cSize, L'#');
    mbstowcs(&wc[0], ip, cSize);

    // Add peer item to peer list in Mouser GUI
    HWND hPeers = getPeerListBox();
    int index = SendMessage(hPeers, LB_ADDSTRING, 0, (LPARAM)wc.c_str());
    SendMessage(hPeers, LB_SETITEMDATA, (WPARAM)index, (LPARAM)this);
    */

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

void Peer::getStreamOpen(Packet* pkt)
{

}

void Peer::getStreamClose(Packet* pkt)
{

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

            // TODO: Figure out why I can't free this memory
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