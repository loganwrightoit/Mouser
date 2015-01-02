#include "stdafx.h"
#include "Mouser.h"
#include "stdafx.h"
#include "ws2tcpip.h"
#include "NetworkManager.h"
#include <iostream>
#include <algorithm>

using namespace std;

#define PEER_LISTEN_PORT 41920
const int DEFAULT_BUFFER_SIZE = 1464;

/*
Multicast address ranges
----------------------------------
224.0.0.0 to 224.0.0.255 (224.0.0.0/24) are reserved for local subnet multicast traffic.
224.0.1.0 to 238.255.255.255 are known as globally scoped addresses, which means they can be used for multicasting across an intranet or the Internet.
239.0.0.0 to 239.255.255.255 (239.0.0.0/8) are reserved for administratively scoped addresses.
239.192.0.0 to 239.251.255.255 Organization-Local Scope
239.255.0.0 to 239.255.255.255 Site-Local Scope
*/

#define         MCST_ADDR   "239.255.92.163"
#define         MCST_PORT   41921
const int       MCST_TTL = 5; // Set higher to traverse routers
ip_mreq         mreq;

NetworkManager::~NetworkManager()
{
    LeaveMulticastGroup();
}

void NetworkManager::Init(HWND hWnd)
{
    StartWinsock();
    JoinMulticastGroup(hWnd);
    SetupPeerListener(hWnd);
}

USHORT NetworkManager::GetPeerListenerPort()
{
    return PEER_LISTEN_PORT;
}

//
// Initializes Winsock
//
void NetworkManager::StartWinsock()
{
    WSADATA wsaData;
    if (WSAStartup(0x0202, &wsaData) != NO_ERROR) // Winsock 2.2
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Winsock]: Initialization failed with error: %s", WSAGetLastError());
        AddOutputMsg(buffer);
        WSACleanup();
    }
    else
    {
        AddOutputMsg(L"[Winsock]: Initialized.");
    }
}

SOCKET NetworkManager::GetPeerListenerSocket()
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PEER_LISTEN_PORT);
    addr.sin_family = AF_INET;

    if (bind(sock, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: bind() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        return INVALID_SOCKET;
    }

    return sock;
}

//
// Leaves the multicast group and closes the socket.
//
bool NetworkManager::LeaveMulticastGroup()
{
    if (setsockopt(m_mcst_sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Multicast]: setsockopt() IP_DROP_MEMBERSHIP failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(m_mcst_sock);
        return false;
    }
    closesocket(m_mcst_sock);
    return true;
}

//
// Gets a multicast socket.
//
SOCKET NetworkManager::GetMulticastSocket()
{
    /* Create socket */

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == INVALID_SOCKET)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Multicast]: socket() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(sock);
        return INVALID_SOCKET;
    }

    /* Bind socket */

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(MCST_PORT);
    if (bind(sock, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Multicast]: bind() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(sock);
        return INVALID_SOCKET;
    }

    // Set time-to-live
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&MCST_TTL, sizeof(MCST_TTL)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Multicast]: setsockopt() IP_MULTICAST_TTL failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(sock);
        return false;
    }

    // Disable loopback

    bool flag = FALSE;
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&flag, sizeof(flag)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Multicast]: setsockopt() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(sock);
        return INVALID_SOCKET;
    }

    // Join the multicast group

    mreq.imr_multiaddr.s_addr = inet_addr(MCST_ADDR);
    mreq.imr_interface.s_addr = INADDR_ANY;
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Multicast]: setsockopt() IP_ADD_MEMBERSHIP failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(sock);
        return INVALID_SOCKET;
    }

    return sock;
}

//
// Sends a multicast packet.
//
bool NetworkManager::SendMulticast(char * identifier)
{
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(MCST_ADDR);
    addr.sin_port = htons(MCST_PORT);

    string s = "MouserMulticast|";
    s.append(identifier);

    //char * buffer = new char[s.length()];
    if (sendto(m_mcst_sock, s.c_str(), s.length(), 0, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Multicast]: sendto() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        return false;
    }
    else
    {
        AddOutputMsg(L"[Multicast]: Discovery packet sent.");
        return true;
    }
}

//
// Receives a multicast packet and returns the host.
//
sockaddr_in NetworkManager::GetMulticastSenderInfo()
{
    sockaddr_in addr;
    int addrLen = sizeof(addr);
    char buffer[DEFAULT_BUFFER_SIZE] = "";

    if (recvfrom(m_mcst_sock, buffer, sizeof(buffer), 0, (LPSOCKADDR)&addr, &addrLen) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Multicast]: recvfrom() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(m_mcst_sock);
    }
    else
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Multicast]: Received discovery packet from %hs", buffer, inet_ntoa(addr.sin_addr));
        AddOutputMsg(buffer);
    }
    return addr;
}

//
// Transmit bytes over connection.
// Returns true if bytes sent successfully.
//
bool NetworkManager::Send(SOCKET sock, CHAR * inBytes, u_int inSize)
{
    u_int sendSize = inSize + sizeof(inSize);
    char *toSend = new char[sendSize];

    // Prepend message with four-byte length header
    toSend[0] = (inSize & 0xff000000) >> 24;
    toSend[1] = (inSize & 0xff0000) >> 16;
    toSend[2] = (inSize & 0xff00) >> 8;
    toSend[3] = (inSize & 0xff);

    // Append data
    memcpy_s(toSend + 4, sendSize, inBytes, inSize);

    // Send the data
    int remaining = sendSize;
    int total = 0;
    while (remaining > 0)
    {
        int result = send(sock, toSend + total, (std::min)(remaining, DEFAULT_BUFFER_SIZE), 0);
        bool blocking = WSAGetLastError() == WSAEWOULDBLOCK;
        if (result == SOCKET_ERROR && !blocking)
        {
            wchar_t buffer[256];
            swprintf(buffer, 256, L"[P2P]: send() failed with error: %i", WSAGetLastError());
            AddOutputMsg(buffer);
            delete[] toSend;
            return false;
        }
        if (!blocking)
        {
            total += result;
            remaining -= result;
        }
    }

    // DEBUG
    //wchar_t buffer[256];
    //swprintf(buffer, 256, L"[P2P]: Sent %i bytes to peer.", total);
    //AddOutputMsg(buffer);

    delete[] toSend;
    return true;
}

//
// Gets length header (first four bytes).
// Call Receive() after making this call to get char array.
//
u_int NetworkManager::GetReceiveLength(SOCKET sock)
{
    u_int szRef = 0;
    int result = recv(sock, (char*)&szRef, sizeof(szRef), 0);
    bool blocking = WSAGetLastError() == WSAEWOULDBLOCK;
    if (result == SOCKET_ERROR && !blocking)
    {
        return result;
    }

    return ntohl(szRef);
}

//
// Receive byte stream.
//
bool NetworkManager::Receive(SOCKET sock, char * inBuffer, u_int recvLength)
{
    int total = 0;
    while (total < recvLength)
    {
        int size = (std::min)((int)(recvLength - total), DEFAULT_BUFFER_SIZE);
        int result = recv(sock, (char*)(inBuffer + total), size, 0);
        bool blocking = WSAGetLastError() == WSAEWOULDBLOCK;
        if (result == SOCKET_ERROR && !blocking)
        {
            wchar_t buffer1[256];
            swprintf(buffer1, 256, L"[P2P]: recv() failed with error: %i", WSAGetLastError());
            AddOutputMsg(buffer1);
            return false;
        }
        if (!blocking)
        {
            total += result;
        }
    }

    return true;
}



//
// Sets blocking mode of socket.
// TRUE blocks, FALSE is non-blocking
//
void NetworkManager::SetBlocking(SOCKET sock, bool block)
{
    block = !block;
    unsigned long mode = block;
    if (ioctlsocket(sock, FIONBIO, &mode) != NO_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Socket]: ioctlsocket() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
    }
}

//
// Sets up UDP multicast listener.
//
void NetworkManager::JoinMulticastGroup(HWND hWnd)
{
    m_mcst_sock = GetMulticastSocket();
    if (m_mcst_sock != INVALID_SOCKET)
    {
        if (WSAAsyncSelect(m_mcst_sock, hWnd, WM_MCST_SOCKET, FD_READ) == SOCKET_ERROR)
        {
            wchar_t buffer[256];
            swprintf(buffer, 256, L"[Multicast]: WSAAsyncSelect() failed with error: %i", WSAGetLastError());
            AddOutputMsg(buffer);
            LeaveMulticastGroup();
        }
    }
}

//
// Sets up TCP connection listener.
//
void NetworkManager::SetupPeerListener(HWND hWnd)
{
    m_p2p_lstn_sock = socket(PF_INET, SOCK_STREAM, 0);

    if (m_p2p_lstn_sock == INVALID_SOCKET)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: socket() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        return;
    }
    if (WSAAsyncSelect(m_p2p_lstn_sock, hWnd, WM_P2P_LISTEN_SOCKET, (FD_CONNECT | FD_ACCEPT | FD_CLOSE)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: WSAAsyncSelect() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(m_p2p_lstn_sock);
        return;
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PEER_LISTEN_PORT);

    if (::bind(m_p2p_lstn_sock, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: bind() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(m_p2p_lstn_sock);
        return;
    }
    if (listen(m_p2p_lstn_sock, 5) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: listen() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(m_p2p_lstn_sock);
        return;
    }
}