#include "stdafx.h"
#include <iostream>
#include <ws2tcpip.h>
#include "ConnectionUtil.h"
#include "Mouser.h"

using namespace std;

#pragma comment (lib, "Ws2_32.lib")

// General
#define         DEFAULT_BUFFER_SIZE 1024
WSADATA         wsaData;
SOCKADDR_IN     client_addr_in;

/*
Mulicast addresses
224.0.0.0 to 224.0.0.255 (224.0.0.0/24) are reserved for local subnet multicast traffic.
224.0.1.0 to 238.255.255.255 are known as globally scoped addresses, which means they can be used for multicasting across an intranet or the Internet.
239.0.0.0 to 239.255.255.255 (239.0.0.0/8) are reserved for administratively scoped addresses.
239.192.0.0 to 239.251.255.255 Organization-Local Scope
239.255.0.0 to 239.255.255.255 Site-Local Scope
*/

// Multicast
#define         MCST_ADDR   "239.255.92.163"
#define         MCST_PORT   41921
const int       MCST_TTL =  5; // Set higher to traverse routers
struct ip_mreq  mreq;

// Broadcast
#define         BCST_PORT   41922

// Default connection port
#define         CONN_PORT   41920

void CloseSocket(SOCKET sock)
{
    shutdown(sock, SD_SEND);
    closesocket(sock);
}

SOCKET GetBroadcastSocket()
{
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    char broadcast = 'a';
    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) != 0)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Broadcast]: setsockopt() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        return INVALID_SOCKET;
    }

    sockaddr_in bcst_rcv_addr;
    bcst_rcv_addr.sin_family = AF_INET;
    bcst_rcv_addr.sin_port = htons(BCST_PORT); // Handle endianness
    bcst_rcv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (LPSOCKADDR)&bcst_rcv_addr, sizeof(bcst_rcv_addr)) == SOCKET_ERROR)
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Broadcast]: bind() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        return INVALID_SOCKET;
    }

    return s;
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

    sockaddr_in bcst_xmt_addr;
    bcst_xmt_addr.sin_family = AF_INET;
    bcst_xmt_addr.sin_port = htons(BCST_PORT); // Handle endianness
    bcst_xmt_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    char * msg = "MouserClient Broadcast";
    if (sendto(sock, msg, strlen(msg) + 1, 0, (sockaddr *)&bcst_xmt_addr, sizeof(bcst_xmt_addr)) < 0)
	{
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Broadcast]: sendto() failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
        return false;
	}
	else
	{
        AddOutputMsg(L"[Broadcast]: Packet sent.");
	}

	return true;
}

bool ReceiveBroadcast(SOCKET sock)
{
    sockaddr_in bcst_from_addr;
    int addrLen = sizeof(bcst_from_addr);

    char szHostName[255];
    gethostname(szHostName, 255);
    struct hostent *host_entry = gethostbyname(szHostName);
    char * szLocalIP = inet_ntoa(*(struct in_addr *)*host_entry->h_addr_list);

    char buffer[DEFAULT_BUFFER_SIZE] = "";
    if (recvfrom(sock, buffer, sizeof(buffer), 0, reinterpret_cast<struct sockaddr*>(&bcst_from_addr), &addrLen))
    {
        if (strcmp(inet_ntoa(bcst_from_addr.sin_addr), szLocalIP) != 0)
        {
            char * inIp = inet_ntoa(bcst_from_addr.sin_addr);
            wchar_t buffer[256];
            swprintf(buffer, 256, L"[Broadcast]: Received broadcast packet from %hs", inIp);
            AddOutputMsg(buffer);
            return true;
        }
    }

    return false;
}

//
// Leaves the multicast group and closes the socket.
//
bool CloseMulticast(SOCKET sock)
{
	if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mreq, sizeof(mreq)))
    {
        wchar_t buffer[256];
        swprintf(buffer, 256, L"[Multicast]: setsockopt() IP_DROP_MEMBERSHIP failed with error: %i", WSAGetLastError());
        AddOutputMsg(buffer);
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
		return INVALID_SOCKET;
	}

	/* Bind socket */

	sockaddr_in mcst_addr;
	mcst_addr.sin_family = AF_INET;
    mcst_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    mcst_addr.sin_port = htons(MCST_PORT);
    if (bind(sock, (struct sockaddr*) &mcst_addr, sizeof(mcst_addr))) {
		AddOutputMsg(L"[Multicast]: bind socket failed.");
		closesocket(sock);
		return INVALID_SOCKET;
	}

	// Set time-to-live

	SetMulticastTTL(sock, MCST_TTL);

	// Disable loopback

	bool flag = FALSE;
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&flag, sizeof(flag))) {
		AddOutputMsg(L"[Multicast]: setsockopt() IP_MULTICAST_LOOP failed.");
		closesocket(sock);
		return INVALID_SOCKET;
	}

	// Join the multicast group

	mreq.imr_multiaddr.s_addr = inet_addr(MCST_ADDR);
	mreq.imr_interface.s_addr = INADDR_ANY;
	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq))) {
		AddOutputMsg(L"[Multicast]: setsockopt() IP_ADD_MEMBERSHIP failed.");
		closesocket(sock);
		return INVALID_SOCKET;
	}

    return sock;
}

//
// Sets time-to-live for Multicast socket.
//
bool SetMulticastTTL(SOCKET sock, int TTL)
{
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&TTL, sizeof(TTL))) {
		AddOutputMsg(L"[Multicast]: setsockopt() IP_MULTICAST_TTL failed.");
		closesocket(sock);
		return false;
	}

	return true;
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
	int result = sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr*) &addr, sizeof(addr));
	if (result < 0) {
		AddOutputMsg(L"[Multicast]: sendto() failed.");
		return false;
	}
	else
	{
		AddOutputMsg(L"[Multicast]: Packet sent.");
		return true;
	}
}

//
// Receives a multicast packet.
//
bool ReceiveMulticast(SOCKET sock)
{
	sockaddr_in addr;
	int addrLen = sizeof(addr);
	char buffer[DEFAULT_BUFFER_SIZE] = "";

	if (recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*) &addr, &addrLen) < 0)
	{
		AddOutputMsg(L"[Multicast]: recvfrom() failed.");
		closesocket(sock);
		return false;
	}
	else
	{
		AddOutputMsg(L"[Multicast]: Received a broadcast from another client.");
		return true;
		//printf("Received multicast from %s:%d: %s\n", inet_ntoa(mcst_rcv_addr.sin_addr), ntohs(mcst_rcv_addr.sin_port), buffer);
	}
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
    int result = send(sock, s.c_str(), s.length(), 0);
    if (result == SOCKET_ERROR)
    {
        cout << "DEBUG: Error requesting: " << WSAGetLastError() << ", shutting down connection." << endl;
        CloseSocket(sock);
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
        cout << "DEBUG: Error receiving: " << WSAGetLastError() << ", shutting down connection." << endl;
        CloseSocket(sock);
    }

    return false;
}

//
// Initializes Winsock
//
bool InitWinsock()
{
    int result = WSAStartup(0x0202, &wsaData); // Winsock 2.2
    if (result != NO_ERROR)
    {
        AddOutputMsg(L"[Winsock]: Encountered error during initialization.");
        return false;
    }
    else
    {
        AddOutputMsg(L"[Winsock]: Successfully initialized.");
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
    int result = ioctlsocket(sock, FIONBIO, &mode);
    if (result != NO_ERROR)
    {
        cout << "ERORR failed setting socket blocking mode: " << result << endl;
    }
}

//
// Opens a peer-to-peer connection with another client.
//  Params:
//      host - IP address of server
//      port - Port of server
//      timeoutSec - Timeout for connection attempt in seconds
//  Return:
//      a SOCKET pointer
//
SOCKET ConnectTo(char * host, u_short port, int timeoutSec)
{
    TIMEVAL Timeout;
    Timeout.tv_sec = timeoutSec;
    Timeout.tv_usec = 0;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in address;
    address.sin_addr.s_addr = inet_addr(host);
    address.sin_port = htons(port);
    address.sin_family = AF_INET;

    // Set to non-blocking and start a connection attempt

    SetBlocking(sock, false);
    connect(sock, (struct sockaddr *)&address, sizeof(address));

    fd_set Write, Err;
    FD_ZERO(&Write);
    FD_ZERO(&Err);
    FD_SET(sock, &Write);
    FD_SET(sock, &Err);

    // Set back to blocking and check if socket is ready after timeout

    SetBlocking(sock, true);
    select(0, NULL, &Write, &Err, &Timeout);
    if (FD_ISSET(sock, &Write))
    {
        SetBlocking(sock, false);
        return sock;
    }
    else {
        return INVALID_SOCKET;
    }
}