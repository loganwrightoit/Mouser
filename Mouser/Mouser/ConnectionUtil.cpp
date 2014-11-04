#include "stdafx.h"
#include <iostream>
#include <ws2tcpip.h>
#include "ConnectionUtil.h"
#include "Mouser.h"

using namespace std;

#pragma comment (lib, "Ws2_32.lib")

#define         DEFAULT_BUFFER_SIZE 256
#define         MAX_BUFFER_SIZE 1456
#define         DEFAULT_PORT 41920
WSADATA         wsaData;
char            myIp[256];

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

char * GetMyHost()
{
    return myIp;
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

SOCKET GetBroadcastSocket()
{
    SOCKET sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    char broadcast = 'a';
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Broadcast]: setsockopt() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(sock);
        return INVALID_SOCKET;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(BCST_PORT); // Handle endianness
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Broadcast]: bind() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        closesocket(sock);
        return INVALID_SOCKET;
    }

    return sock;
}

//
// Sends a UDP broadcast packet across local network.
//
bool SendBroadcast(SOCKET sock)
{
    if (sock == INVALID_SOCKET)
    {
        AddOutputMsg(L"[Broadcast]: sendto() invalid socket.");
        return false;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(BCST_PORT); // Handle endianness
    addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    char * msg = "MouserClient Broadcast";
    if (sendto(sock, msg, strlen(msg), 0, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Broadcast]: sendto() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        return false;
	}
	else
	{
        AddOutputMsg(L"[Broadcast]: Discovery packet sent.");
	}

	return true;
}

sockaddr_in GetBroadcastSenderInfo(SOCKET sock)
{
    sockaddr_in addr;
    int size = sizeof(addr);

    char buffer[DEFAULT_BUFFER_SIZE] = "";
    if (recvfrom(sock, buffer, sizeof(buffer), 0, (LPSOCKADDR)&addr, &size) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Broadcast]: recvfrom() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
    }

    return addr;
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
bool SendMulticast(SOCKET sock)
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(MCST_ADDR);
	addr.sin_port = htons(MCST_PORT);

	char * buffer = "MouserMulticast Packet";
	if (sendto(sock, buffer, strlen(buffer), 0, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)    
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
        swprintf(buffer, 256, L"[Multicast]: Received discovery packet from %hs", inet_ntoa(addr.sin_addr));
        AddOutputMsg(buffer);
	}
    return addr;
}

//
// Transmit message over connection.
// Returns true if message successfully sent.
//
bool Send(SOCKET sock, CHAR * inBytes)
{
    // Prepend message with two-byte length header
    u_short len = strlen(inBytes);
    BYTE byteLen[] = { (len & 0xff00) >> 8, (len & 0xff) };
    string s = "";
    s += byteLen[0];
    s += byteLen[1];
    s += inBytes;

    // Transmit remaining bytes
    if (send(sock, s.c_str(), s.length(), 0) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: send() failed with error: %s", WSAGetLastError());
        AddOutputMsg(buffer);
        return false;
    }
    else
    {
        return true;
    }
}

//
// Receive transaction data and fill outBytes.
//
bool Receive(SOCKET sock, char * outBytes)
{
    string s = "";

    // Receive first two bytes (message length)
    u_short messageLength;
    int inBytes = recv(sock, (char*)&messageLength, sizeof(messageLength), 0);

    if (inBytes > 0) // Receive all the data
    {
        int bytesLeft = inBytes;
        do {
            // Receive all the data
			char buffer[DEFAULT_BUFFER_SIZE] = "";
            bytesLeft -= recv(sock, (char*)&buffer, sizeof(buffer), 0);
            s.append(buffer);
        } while (bytesLeft > 0);

        strcpy_s(outBytes, sizeof(outBytes), const_cast<char*>(s.c_str()));
        return true;
    }
    else if (inBytes != WSAEWOULDBLOCK && inBytes != 0) // Nothing to receive, don't block
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[P2P]: recv() failed with error: %s", WSAGetLastError());
        AddOutputMsg(buffer);
        CloseSocket(sock);
    }

    return false;
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

        // Set my IP
        char szHostName[255];
        gethostname(szHostName, 255);
        struct hostent *host_entry = gethostbyname(szHostName);
        strcpy_s(myIp, inet_ntoa(*(struct in_addr *)*host_entry->h_addr_list));
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