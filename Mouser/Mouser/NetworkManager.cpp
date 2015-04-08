#include "stdafx.h"
#include "NetworkManager.h"
#include "ws2tcpip.h"
#include <algorithm>
#include <string>

/*
Multicast address ranges
----------------------------------
224.0.0.0 to 224.0.0.255 (224.0.0.0/24) are reserved for local subnet multicast traffic.
224.0.1.0 to 238.255.255.255 are known as globally scoped addresses, which means they can be used for multicasting across an intranet or the Internet.
239.0.0.0 to 239.255.255.255 (239.0.0.0/8) are reserved for administratively scoped addresses.
239.192.0.0 to 239.251.255.255 Organization-Local Scope
239.255.0.0 to 239.255.255.255 Site-Local Scope
*/

const char *         MCST_IP             = "239.255.92.163";
const unsigned int   MCST_PORT           = 41921;
const int            MCST_TTL            = 5; // Set higher to traverse routers
const unsigned short PEER_LISTEN_PORT    = 41920;
//const int            DEFAULT_BUFFER_SIZE = 1464; // Moved to Mouser.h for global access
struct ip_mreq       mreq;

void NetworkManager::init(const HWND hWnd)
{
    startWinsock();
    joinMulticastGroup(hWnd);
    setupPeerListener(hWnd);
}

unsigned short NetworkManager::getPeerListenerPort() const
{
    return PEER_LISTEN_PORT;
}

void NetworkManager::getIP(wchar_t* outString)
{
    char ac[80];
    if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR)
    {
        return;
    }

    struct hostent *phe = gethostbyname(ac);
    if (phe == 0)
    {
        return;
    }
    
    struct in_addr addr;
    memcpy(&addr, phe->h_addr_list[0], sizeof(struct in_addr));

    char* ip = inet_ntoa(addr);

    // Convert to a wchar_t*
    size_t origsize = strlen(ip) + 1;
    const size_t newsize = 100;
    size_t convertedChars = 0;
    wchar_t wcstring[newsize];
    mbstowcs_s(&convertedChars, wcstring, origsize, ip, _TRUNCATE);

    wcscpy_s(outString, 80, wcstring);
}

//
// Initializes Winsock
//
void NetworkManager::startWinsock()
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

SOCKET NetworkManager::getPeerListenerSocket() const
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
bool NetworkManager::leaveMulticastGroup()
{
    if (setsockopt(_mcst_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Multicast]: setsockopt() IP_DROP_MEMBERSHIP failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(_mcst_socket);
        return false;
    }
    closesocket(_mcst_socket);
    return true;
}

//
// Gets a multicast socket.
//
SOCKET NetworkManager::getMulticastSocket() const
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

    mreq.imr_multiaddr.s_addr = inet_addr(MCST_IP);
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
bool NetworkManager::sendMulticast(char * identifier)
{
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(MCST_IP);
    addr.sin_port = htons(MCST_PORT);

    std::string s = "MouserMulticast|";
    s.append(identifier);

    //char * buffer = new char[s.length()];
    if (sendto(_mcst_socket, s.c_str(), s.length(), 0, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
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
sockaddr_in NetworkManager::getMulticastSenderInfo() const
{
    sockaddr_in addr;
    int addrLen = sizeof(addr);
    char buffer[DEFAULT_BUFFER_SIZE] = "";

    if (recvfrom(_mcst_socket, buffer, sizeof(buffer), 0, (LPSOCKADDR)&addr, &addrLen) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Multicast]: recvfrom() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(_mcst_socket);
    }
    else
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Multicast]: Received discovery packet from %hs", inet_ntoa(addr.sin_addr));
        AddOutputMsg(buffer);
    }
    return addr;
}

//
// Transmit bytes over connection.
// Returns true if bytes sent successfully.
//
bool NetworkManager::sendPacket(SOCKET socket, Packet * pkt)
{
    // Cache values
    unsigned int dataSize = pkt->getSize();
    Packet::Protocol protocol = pkt->getProtocol();

    unsigned int lengthHeader = sizeof(protocol) + dataSize;
    unsigned int sendSize = sizeof(unsigned int) /*Length header*/ + sizeof(protocol) + dataSize;
    char * toSend = new char[sendSize];

    // Prepend message with four-byte length header
    toSend[0] = (lengthHeader & 0xff000000) >> 24;
    toSend[1] = (lengthHeader & 0xff0000) >> 16;
    toSend[2] = (lengthHeader & 0xff00) >> 8;
    toSend[3] = (lengthHeader & 0xff);

    // Prepend protocol
    toSend[4] = (protocol & 0xff000000) >> 24;
    toSend[5] = (protocol & 0xff0000) >> 16;
    toSend[6] = (protocol & 0xff00) >> 8;
    toSend[7] = (protocol & 0xff);

    // Append data
	// Not zero is error condition
	if (memcpy_s(toSend + 8, sendSize, pkt->getData(), pkt->getSize()))
	{
		delete[] toSend;
		AddOutputMsg(L"[ERROR]: sendPacket memcpy_s failed, aborting.");
		return false;
	}

    // Send the data
    int remaining = sendSize;
    int total = 0;
    while (remaining > 0)
    {
        int result = send(socket, toSend + total, (std::min)(remaining, DEFAULT_BUFFER_SIZE), 0);
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

    delete[] toSend;
    return true;
}

bool NetworkManager::isSocketReady(SOCKET sock, int channel) const
{
    fd_set sready;
    struct timeval nowait;

    FD_ZERO(&sready);
    FD_SET((unsigned int)sock, &sready);
    memset((char *)&nowait, 0, sizeof(nowait));
	bool result = false;
	if (select(sock, (channel == FD_READ ? &sready : NULL), (channel == FD_WRITE ? &sready : NULL), NULL, &nowait))
	{
		result = true;
	}

    if (FD_ISSET(sock, &sready))
		result = true;
    else
		result = false;


	return result;
}

//
// Blocks until data is received, returning a MouserPacket.
// Delete packet instance when done processing data.
//
Packet * NetworkManager::getPacket(SOCKET socket)
{
    // Grab length of message (four bytes, usually)

    int size = 0;
    int result = recv(socket, (char*)&size, sizeof(size), 0);
    bool blocking = WSAGetLastError() == WSAEWOULDBLOCK;
    if (blocking)
    {
        AddOutputMsg(L"[ERROR]: doRcv() could not receive packet, would block.");
        return nullptr;
    }

    int bytesRemaining = ntohl(size);

    if (result == SOCKET_ERROR && !blocking)
    {
        // Remove peer if connection dropped
        if (bytesRemaining == 0)
        {
            return new Packet(Packet::Protocol::DISCONNECT, 0, 0);
        }
    }

    // Grab protocol (four bytes)

    int rawProtocol = 0;
    int result1 = recv(socket, (char*)&rawProtocol, sizeof(rawProtocol), 0);
    Packet::Protocol protocol = (Packet::Protocol) ntohl(rawProtocol);
    bool blocking1 = WSAGetLastError() == WSAEWOULDBLOCK;
    if (result1 == SOCKET_ERROR && !blocking1)
    {
        // Handle error if needed
    }
    bytesRemaining -= sizeof(protocol);

    // Grab remaining data
    char * data = new char[bytesRemaining];
    doRcv(socket, data, bytesRemaining);

    Packet * pkt = new Packet(protocol, data, bytesRemaining);
    return pkt;
}

//
// Receive byte stream.
//
bool NetworkManager::doRcv(SOCKET socket, char * data, unsigned int recvLength)
{
    unsigned int total = 0;
    while (total < recvLength)
    {
        unsigned int size = (std::min)((int)(recvLength - total), DEFAULT_BUFFER_SIZE);
        unsigned int result = recv(socket, (char*)(data + total), size, 0);
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
void NetworkManager::setBlocking(SOCKET socket, bool block)
{
    block = !block;
    unsigned long mode = block;
    if (ioctlsocket(socket, FIONBIO, &mode) != NO_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Socket]: ioctlsocket() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
    }
}

//
// Sets up UDP multicast listener.
//
void NetworkManager::joinMulticastGroup(HWND hWnd)
{
    _mcst_socket = getMulticastSocket();
    if (_mcst_socket != INVALID_SOCKET)
    {
        if (WSAAsyncSelect(_mcst_socket, hWnd, WM_MCST_SOCKET, FD_READ) == SOCKET_ERROR)
        {
            wchar_t buffer[256];
            swprintf(buffer, 256, L"[Multicast]: WSAAsyncSelect() failed with error: %i", WSAGetLastError());
            AddOutputMsg(buffer);
            leaveMulticastGroup();
        }
    }
}

//
// Sets up TCP connection listener.
//
void NetworkManager::setupPeerListener(HWND hWnd)
{
    _p2p_lstn_socket = socket(PF_INET, SOCK_STREAM, 0);

    if (_p2p_lstn_socket == INVALID_SOCKET)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: socket() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        return;
    }
    if (WSAAsyncSelect(_p2p_lstn_socket, hWnd, WM_P2P_LISTEN_SOCKET, (FD_CONNECT | FD_ACCEPT | FD_CLOSE)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: WSAAsyncSelect() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(_p2p_lstn_socket);
        return;
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PEER_LISTEN_PORT);

    if (::bind(_p2p_lstn_socket, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: bind() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(_p2p_lstn_socket);
        return;
    }
    if (listen(_p2p_lstn_socket, 5) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: listen() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(_p2p_lstn_socket);
        return;
    }
}