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

// Multicast
#define         MCST_ADDR   "234.241.92.163"
#define         MCST_PORT   41921
const int       MCST_TTL = 2; // Set higher to traverse routers
SOCKET          mcst_sock = INVALID_SOCKET; // IGMP Multicast Socket
SOCKADDR_IN     mcst_addr;
IP_MREQ         mcst_interface;

// Broadcast
#define         BCST_PORT   41922

#define         CONN_PORT   41900

void CloseSocket(SOCKET sock)
{
    shutdown(sock, SD_SEND);
    closesocket(sock);
}

SOCKET GetBcstListenSocket()
{
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    char broadcast = 'a';
    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) != 0) {
        AddOutputMsg(L"[Broadcast]: Unable to set socket options.");
        return INVALID_SOCKET;
    }

    sockaddr_in bcst_rcv_addr;
    bcst_rcv_addr.sin_family = AF_INET;
    bcst_rcv_addr.sin_port = htons(BCST_PORT); // Handle endianness
    bcst_rcv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (LPSOCKADDR)&bcst_rcv_addr, sizeof(bcst_rcv_addr)) == SOCKET_ERROR)
    {
        AddOutputMsg(L"[Broadcast]: Unable to bind socket.");
        return INVALID_SOCKET;
    }

    return s;
}

//
// Sends a UDP broadcast packet across local network.
//
bool SendUdpBroadcast(SOCKET sock)
{
    if (sock == INVALID_SOCKET)
    {
        AddOutputMsg(L"[Broadcast]: SendUdpBroacast() passed in invalid socket.");
        return false;
    }

    sockaddr_in bcst_xmt_addr;
    bcst_xmt_addr.sin_family = AF_INET;
    bcst_xmt_addr.sin_port = htons(BCST_PORT); // Handle endianness
    bcst_xmt_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    char * msg = "MouserClient Broadcast";
    if (sendto(sock, msg, strlen(msg) + 1, 0, (sockaddr *)&bcst_xmt_addr, sizeof(bcst_xmt_addr)) < 0)
	{
        AddOutputMsg(L"[Broadcast]: SendUdpBroadcast() - unable to send message.");
        return false;
	}
	else
	{
        AddOutputMsg(L"[Broadcast]: SendUdpBroadcast() - message sent.");
	}

	return true;
}

bool CloseMulticast()
{
    if (setsockopt(mcst_sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mcst_interface, sizeof(mcst_interface))) {
		//printf("setsockopt() IP_DROP_MEMBERSHIP address %s failed: %d\n", mCastIp, WSAGetLastError());
        return false;
	}
    closesocket(mcst_sock);
    AddOutputMsg(L"[Multicast]: Socket closed.");
    return true;
}

SOCKET GetMcstListenSocket()
{
    /*
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in localAddress;
    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(CONN_PORT);
    localAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (LPSOCKADDR)&localAddress, sizeof(localAddress)) == SOCKET_ERROR)
    {
        AddOutputMsg(L"[Broadcast]: Unable to bind listener socket.");
        return INVALID_SOCKET;
    }

    return s;
    */

    ////// ACTUAL CODE BELOW

	BOOL flag;
	int result;

	/* Assign socket */

	mcst_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (mcst_sock == INVALID_SOCKET) {
		printf("socket() failed: %d\n", WSAGetLastError());
		WSACleanup();
		exit(1);
	}

	/* Bind socket */

	mcst_addr.sin_family = AF_INET;
    mcst_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    mcst_addr.sin_port = htons(MCST_PORT);
    if (bind(mcst_sock, (struct sockaddr*) &mcst_addr, sizeof(mcst_addr))) {
		printf("bind() port: %d failed: %d\n", MCST_PORT, WSAGetLastError());
	}

	/* Set time-to-live if not default size */

	if (MCST_TTL > 1) {
        if (setsockopt(mcst_sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&MCST_TTL, sizeof(MCST_TTL))) {
			printf("setsockopt() IP_MULTICAST_TTL failed: %d\n", WSAGetLastError());
		}
	}

	/* Disable loopback */

	flag = FALSE;
    if (setsockopt(mcst_sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&flag, sizeof(flag))) {
		printf("setsockopt() IP_MULTICAST_LOOP failed: %d\n", WSAGetLastError());
	}

	/* Assign destination address */

    //mcst_addr.sin_family = AF_INET;
    //mcst_addr.sin_addr.s_addr = inet_addr(MCST_ADDR);
    //mcst_addr.sin_port = htons(MCST_PORT);

	/* Join the multicast group */

	mcst_interface.imr_multiaddr.s_addr = inet_addr(MCST_ADDR);
    mcst_interface.imr_interface.s_addr = INADDR_ANY;
    if (setsockopt(mcst_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mcst_interface, sizeof(mcst_interface))) {
        printf("setsockopt() IP_ADD_MEMBERSHIP address %s failed: %d\n", MCST_ADDR, WSAGetLastError());
	}

    return mcst_sock;
}

//
// Listen for multicasts.
//
bool MulticastListenThread()
{
	SOCKADDR_IN mcst_rcv_addr;
    int addrLen = sizeof(mcst_rcv_addr);
    char buffer[DEFAULT_BUFFER_SIZE] = "";

    if (recvfrom(mcst_sock, buffer, sizeof(buffer), 0, (struct sockaddr*) &mcst_rcv_addr, &addrLen) < 0) {
        closesocket(mcst_sock);
        return false;
	}
	else {
        AddOutputMsg(L"[Multicast]: Received a broadcast from another client.");
        return true;
        //printf("Received multicast from %s:%d: %s\n", inet_ntoa(mcst_rcv_addr.sin_addr), ntohs(mcst_rcv_addr.sin_port), buffer);
	}
}

//
// Sends a multicast broadcast.
//
bool SendMulticastBroadcast(char * inBytes)
{
    int result = sendto(mcst_sock, inBytes, strlen(inBytes), 0, (struct sockaddr*) &mcst_addr, sizeof(mcst_addr));
	if (result < 0) {
		printf("sendto() failed: %d\n", WSAGetLastError());
		WSACleanup();
		exit(1);
	}
}

//
// Client discovery function that listens for UDP broadcasts.
//
void ListenForBroadcast(SOCKET sock)
{
    /*
    SOCKADDR_IN bcst_rcv_addr;
    int addrLen = sizeof(bcst_rcv_addr);

    // Resolve machine IP address

    char szHostName[255];
    gethostname(szHostName, 255);
    HOSTENT * host_entry = gethostbyname(szHostName);
    char * self_host = inet_ntoa(*(struct in_addr *)*host_entry->h_addr_list);

    while (1)
    {
        int addrLen = sizeof(bcst_rcv_addr);
        char buffer[DEFAULT_BUFFER_SIZE] = "";
        if (recvfrom(bcst_sock, buffer, sizeof(buffer), 0, reinterpret_cast<struct sockaddr*>(&bcst_rcv_addr), &addrLen))
        {
            if (strcmp(inet_ntoa(bcst_rcv_addr.sin_addr), self_host) != 0) {
                cout << "Processed recvfrom : " << inet_ntoa(bcst_rcv_addr.sin_addr) << buffer << '\n';
            }
        }
    }
    */
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

bool initialized = false;

bool isInit()
{
    return initialized;
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
        initialized = true;
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
// Opens a client connection to server.
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