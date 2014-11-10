#pragma comment (lib, "Ws2_32.lib")

#include "stdafx.h"
#include "ws2tcpip.h"
#include "ConnectionUtil.h"
#include "Mouser.h"

#include <iostream>
#include <algorithm>

using namespace std;

#define         DEFAULT_PORT 41920
WSADATA         wsaData;

/*
Mulicast addresses
224.0.0.0 to 224.0.0.255 (224.0.0.0/24) are reserved for local subnet multicast traffic.
224.0.1.0 to 238.255.255.255 are known as globally scoped addresses, which means they can be used for multicasting across an intranet or the Internet.
239.0.0.0 to 239.255.255.255 (239.0.0.0/8) are reserved for administratively scoped addresses.
239.192.0.0 to 239.251.255.255 Organization-Local Scope
239.255.0.0 to 239.255.255.255 Site-Local Scope
*/

#define         MCST_ADDR   "239.255.92.163"
#define         MCST_PORT   41921
#define         BCST_PORT   41922
const int       MCST_TTL =  5; // Set higher to traverse routers
ip_mreq         mreq;

USHORT GetPrimaryClientPort()
{
    return DEFAULT_PORT;
}

void CloseSocket(SOCKET sock)
{
    shutdown(sock, SD_SEND);
    closesocket(sock);
}

SOCKET GetConnectionSocket()
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(DEFAULT_PORT);
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
bool CloseMulticast(SOCKET sock)
{
	if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Multicast]: setsockopt() IP_DROP_MEMBERSHIP failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(sock);
        return false;
	}
    closesocket(sock);
    return true;
}

//
// Gets a multicast socket.
//
SOCKET GetMulticastSocket()
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
bool SendMulticast(SOCKET sock, char * identifier)
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(MCST_ADDR);
	addr.sin_port = htons(MCST_PORT);

    string s = "MouserMulticast|";
    s.append(identifier);

	//char * buffer = new char[s.length()];
	if (sendto(sock, s.c_str(), s.length(), 0, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)    
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
sockaddr_in GetMulticastSenderInfo(SOCKET sock)
{
	sockaddr_in addr;
	int addrLen = sizeof(addr);
	char buffer[DEFAULT_BUFFER_SIZE] = "";

    if (recvfrom(sock, buffer, sizeof(buffer), 0, (LPSOCKADDR)&addr, &addrLen) == SOCKET_ERROR)
	{
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Multicast]: recvfrom() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
		closesocket(sock);
	}
	else
	{
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Multicast]: Received discovery packet from %hs at %hs", buffer, inet_ntoa(addr.sin_addr));
        AddOutputMsg(buffer);
	}
    return addr;
}

//
// Transmit bytes over connection.
// Returns true if bytes sent successfully.
//
bool Send(SOCKET sock, CHAR * inBytes, u_int inSize)
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
u_int GetReceiveLength(SOCKET sock)
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
bool Receive(SOCKET sock, char * inBuffer, u_int recvLength)
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
// Initializes Winsock
//
bool InitWinsock()
{
    if (WSAStartup(0x0202, &wsaData) != NO_ERROR) // Winsock 2.2
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Winsock]: Initialization failed with error: %s", WSAGetLastError());
        AddOutputMsg(buffer);
        WSACleanup();
        return false;
    }
    else
    {
        AddOutputMsg(L"[Winsock]: Initialized.");
        return true;
    }
}

//
// Sets blocking mode of socket.
// TRUE blocks, FALSE is non-blocking
//
void SetBlocking(SOCKET sock, bool block)
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