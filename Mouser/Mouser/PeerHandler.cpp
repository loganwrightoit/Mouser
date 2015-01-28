#include "stdafx.h"
#include "PeerHandler.h"
#include <thread>

std::vector<Peer*> PeerHandler::getPeers() const
{
    return peers;
}

int PeerHandler::getNumPeers() const
{
    return peers.size();
}

void PeerHandler::disconnectPeer(Peer * peer)
{
    auto iter = peers.begin();
    while (iter != peers.end())
    {
        if (*iter == peer)
        {
            // Erase peer from vector
            peers.erase(iter);

            sockaddr_in addr;
            int size = sizeof(addr);
            getpeername(peer->getSocket(), (sockaddr*)&addr, &size);
            wchar_t buffer[256];
            swprintf(buffer, 256, L"[P2P]: Peer disconnected at %hs.", inet_ntoa(addr.sin_addr));
            AddOutputMsg(buffer);

            delete peer;
        }
    }

    // Update main GUI peer listbox
    updatePeerListBoxData();
}

Peer * PeerHandler::getPeer(int idx) const
{
    if (peers.size() >= idx)
    {
        return peers.at(idx);
    }
    else
    {
        return nullptr;
    }
}

/*
void PeerHandler::ListenToPeerThread()
{
    AddOutputMsg(L"[DEBUG]: Listening for peer data...");

    while (m_peer_sock != INVALID_SOCKET)
    {
        u_int length = network->GetReceiveLength(m_peer_sock);
        bool blocking = WSAGetLastError() == WSAEWOULDBLOCK;
        if (length == SOCKET_ERROR && !blocking)
        {
            // Socket was shutdown already, exit thread
            return;
        }
        else if (!blocking)
        {
            if (length == 0)
            {
                AddOutputMsg(L"[P2P]: Peer disconnected.");
                //::EnableWindow(hDisconnectPeerButton, false);
                //::EnableWindow(hSendPeerDataButton, false);
                //::EnableWindow(hCaptureScreenButton, false);
                closesocket(m_peer_sock);
                p2p_sock = INVALID_SOCKET;
                return;
            }
            else
            {
                // Receive remainder of message
                char * buffer = new char[length];
                if (network->Receive(m_peer_sock, buffer, length))
                {
                    // Client should send a stream start packet, showing window ONCE
                    ShowWindow(hStreamWindow, SW_SHOW);

                    // For now, assume data is CImage stream data
                    IStream *pStream;
                    HRESULT result = CreateStreamOnHGlobal(0, TRUE, &pStream);
                    IStream_Write(pStream, buffer, length);

                    // Create image from stream
                    CImage image;
                    image.Load(pStream);

                    // Adjust window size if set to image size
                    RECT strRect;
                    GetWindowRect(hStreamWindow, &strRect);
                    int strWidth = strRect.right - strRect.left;
                    int strHeight = strRect.top - strRect.bottom;

                    if (image.GetWidth() != strWidth || image.GetHeight() != strHeight)
                    {
                        MoveWindow(hStreamWindow, strRect.left, strRect.top, image.GetWidth(), image.GetHeight(), false);
                        CenterWindow(hStreamWindow);
                    }

                    // Draw image to screen
                    HDC hdc = GetDC(hStreamWindow);
                    DrawImage(hdc, image); // Uses stretch blt method
                    //image.BitBlt(hdc, 0, 0);

                    ReleaseDC(hStreamWindow, hdc);
                    pStream->Release();

                    // Draw static cursor on screen after blit
                    // Always get last location from a cache in case update has not occurred
                    HICON NormalCursor = (HICON)LoadImage(NULL, MAKEINTRESOURCE(IDC_ARROW), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
                    HDC hDC = GetDC(hStreamWindow);
                    DrawIconEx(hDC, streamLastCursorLocX, streamLastCursorLocY, NormalCursor, 0, 0, NULL, NULL, DI_DEFAULTSIZE | DI_NORMAL);
                }
                delete[] buffer;
            }
        }
    }
}
*/

void PeerHandler::connectToPeer()
{
    sockaddr_in inAddr = NetworkManager::getInstance().getMulticastSenderInfo();
    std::thread t(&PeerHandler::connectToPeerThread, this, inAddr);
    t.detach();
}

void PeerHandler::connectToPeerThread(sockaddr_in inAddr)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: socket() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        return;
    }

    // Set up our socket address structure
    SOCKADDR_IN addr;
    addr.sin_port = htons(NetworkManager::getInstance().getPeerListenerPort());
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inAddr.sin_addr.S_un.S_addr;

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: connect() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(sock);
        return;
    }

    // Add peer to vector
    peers.push_back(new Peer(sock));

    // Update main GUI peer listbox
    updatePeerListBoxData();
}

void PeerHandler::handlePeerConnectionRequest(WPARAM wParam)
{
    SOCKET sock = accept(wParam, NULL, NULL);
    if (sock == INVALID_SOCKET)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: accept() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        return;
    }

    // Add peer
    peers.push_back(new Peer(sock));

    // Update main GUI peer listbox
    updatePeerListBoxData();
}