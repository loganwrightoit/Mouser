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

            printf("[P2P]: Peer disconnected at %hs\n", inet_ntoa(addr.sin_addr));
            
            // Stop peer stream
            peer->onDestroyRoot();

            // Erase peer from vector
            peers.erase(iter);
        }
    }

    // Update main GUI peer listbox
    updatePeerListBoxData();
}

void PeerHandler::directConnectToPeer(char* ip)
{
	// Set up our socket address structure
	SOCKADDR_IN addr;
	addr.sin_port = htons(NetworkManager::getInstance().getPeerListenerPort());
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);

    printf("[P2P]: Attempting peer connection at %hs\n", inet_ntoa(addr.sin_addr));

	std::thread t(&PeerHandler::connectToPeerThread, this, addr);
	t.detach();
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
        printf("[P2P]: socket() failed with error: %i\n", WSAGetLastError());
        return;
    }

    // Set up our socket address structure
    SOCKADDR_IN addr;
    addr.sin_port = htons(NetworkManager::getInstance().getPeerListenerPort());
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inAddr.sin_addr.S_un.S_addr;

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        printf("[P2P]: connect() failed with error: %i\n", WSAGetLastError());
        closesocket(sock);
        return;
    }

    // Clear windows events on socket, which seem to be inherited
    WSAAsyncSelect(sock, getRootWindow(), 0, 0);

    // Set socket to blocking
    NetworkManager::getInstance().setBlocking(sock, true);

    // Add peer to vector
    peers.push_back(new Peer(sock));

    // Update main GUI peer listbox
    //updatePeerListBoxData();
}

void PeerHandler::handlePeerConnectionRequest(WPARAM wParam)
{
    SOCKET sock = accept(wParam, NULL, NULL);
    if (sock == INVALID_SOCKET)
    {
        printf("[P2P]: accept() failed with error: %i\n", WSAGetLastError());
        return;
    }

    // Clear windows events on socket, which seem to be inherited
    WSAAsyncSelect(sock, getRootWindow(), 0, 0);

    // Set socket to blocking
    NetworkManager::getInstance().setBlocking(sock, true);

    // Add peer
    peers.push_back(new Peer(sock));

    // Update main GUI peer listbox
    //updatePeerListBoxData();
}