#include "stdafx.h"
#include "PeerHandler.h"
#include <thread>

std::vector<Peer*> PeerHandler::getPeers() const
{
    return peers;
}

//
// Returns peer associated with peer window or control.
//
Peer* PeerHandler::getPeer(HWND _In_hWnd)
{
    HWND hWnd = GetAncestor(_In_hWnd, GA_ROOT);
    Peer* peer = (Peer*) GetWindowLongPtr(hWnd, GWL_USERDATA);
    return peer;
}

void PeerHandler::disconnectPeer(Peer * peer)
{
    auto iter = peers.begin();
    while (iter != peers.end())
    {
        if (*iter == peer)
        {
            sockaddr_in addr;
            int size = sizeof(addr);
            getpeername(peer->getSocket(), (sockaddr*)&addr, &size);
            wchar_t buffer[256];
            swprintf(buffer, 256, L"[P2P]: Peer disconnected at %hs.", inet_ntoa(addr.sin_addr));
            AddOutputMsg(buffer);

            // Erase peer from vector
            peers.erase(iter);
        }
    }

    // Update main GUI peer listbox
    updatePeerListBoxData();
}

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