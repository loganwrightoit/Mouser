#include "stdafx.h"
#include <iostream>
#include <ws2tcpip.h>
#include "ConnectionUtil.h"
#include "Mouser.h"

using namespace std;

#pragma comment (lib, "Ws2_32.lib")

// General
#define DEFAULT_BUFFER_SIZE 1024
WSADATA wsaData;
struct SOCKADDR_IN clientService;

// Multicast definitions
#define MCAST_PORT  41921
#define MCAST_IP    "234.241.92.163"
#define MCAST_TTL   2 // Set higher to traverse routers
SOCKET  mcst_sock = INVALID_SOCKET; // IGMP Multicast Socket

// Multicast fields
struct IP_MREQ mCastInterface;
SOCKET         mCastSocket;
char           mCastIp[] = MCAST_IP;
USHORT         mCastPort = MCAST_PORT;
ULONG          mCastTTL = MCAST_TTL;
SOCKADDR_IN    mCastAddr, clientAddr;

// Broadcast defitions
struct sockaddr_in Recv_addr; // for receiving udp broadcast
struct sockaddr_in bcst_addr; // for udp broadcast
SOCKET bcst_sock = INVALID_SOCKET; // UDP Broadcast Socket

void CloseSocket(SOCKET sock)
{
    shutdown(sock, SD_SEND);
    closesocket(sock);
}

//
// Sends a UDP broadcast packet across local network.
//
bool SendUdpBroadcast()
{
	socklen_t recLen = sizeof(Recv_addr);
	char * szLocalIP;

	// Create broadcast socket

	WSADATA wsaData;
	WSAStartup(0x0202, &wsaData);
	bcst_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (bcst_sock == INVALID_SOCKET) {
		closesocket(bcst_sock);
		AddOutputMsg(L"Failed to create broadcast socket.");
		return false;
	}

	char * msg = "MouserClient Broadcast";
	int _result = 0;

	_result = setsockopt(bcst_sock, SOL_SOCKET, SO_BROADCAST, msg, sizeof(msg));
	if (_result != 0) {
		printf("ERROR: setting socket failed: %d\n", _result);
		cleanUp();
		return 1;
	}

	// Resolve machine IP address

	char szHostName[255];
	gethostname(szHostName, 255);
	struct hostent *host_entry = gethostbyname(szHostName);
	szLocalIP = inet_ntoa(*(struct in_addr *)*host_entry->h_addr_list);

	// Setup structs

	Recv_addr.sin_family = AF_INET;
	Recv_addr.sin_port = htons(PORT); // Handle endianness
	Recv_addr.sin_addr.s_addr = INADDR_ANY;

	bcst_addr.sin_family = AF_INET;
	bcst_addr.sin_port = htons(PORT); // Handle endianness
	bcst_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

	if (bind(sock, (sockaddr*)&Recv_addr, sizeof(Recv_addr)) < 0)
	{
		perror("bind");
		_getch();
		closesocket(sock);
		return 1;
	}

	// Send broadcast
	char * msg = "MouserClient";
	if (sendto(sock, msg, strlen(msg) + 1, 0, (sockaddr *)&Sender_addr, sizeof(Sender_addr)) < 0)
	{
		AddOutputMsg(L"Broadcast packet was not able to be sent.");
		_getch();
		closesocket(sock);
	}
	else
	{
		AddOutputMsg(L"Broadcast packet sent.");
	}


	_beginthread(recvFunct, 0, NULL);
	_beginthread(sendFunct1, 0, NULL);

	cout << "spawned threads, press any key to exit.. \n";
	closesocket(sock);
	WSACleanup();
	return 0;
}

void CloseMulticast()
{
	if (setsockopt(mCastSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mCastInterface, sizeof(mCastInterface))) {
		printf("setsockopt() IP_DROP_MEMBERSHIP address %s failed: %d\n", mCastIp, WSAGetLastError());
	}
}

void InitMulticast()
{
	BOOL flag;
	int result;
	WSADATA stWsaData;

	/* Initialize winsock v2.2 */

	result = WSAStartup(0x0202, &stWsaData);
	if (result) {
		printf("WSAStartup failed: %d\r\n", result);
		exit(1);
	}

	/* Assign socket */

	mCastSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (mCastSocket == INVALID_SOCKET) {
		printf("socket() failed: %d\n", WSAGetLastError());
		WSACleanup();
		exit(1);
	}

	/* Bind socket */

	clientAddr.sin_family = AF_INET;
	clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	clientAddr.sin_port = htons(mCastPort);
	if (bind(mCastSocket, (struct sockaddr*) &clientAddr, sizeof(clientAddr))) {
		printf("bind() port: %d failed: %d\n", mCastPort, WSAGetLastError());
	}

	/* Set time-to-live if not default size */

	if (mCastTTL > 1) {
		if (setsockopt(mCastSocket, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&mCastTTL, sizeof(mCastTTL))) {
			printf("setsockopt() IP_MULTICAST_TTL failed: %d\n", WSAGetLastError());
		}
	}

	/* Disable loopback */

	flag = FALSE;
	if (setsockopt(mCastSocket, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&flag, sizeof(flag))) {
		printf("setsockopt() IP_MULTICAST_LOOP failed: %d\n", WSAGetLastError());
	}

	/* Assign destination address */

	mCastAddr.sin_family = AF_INET;
	mCastAddr.sin_addr.s_addr = inet_addr(mCastIp);
	mCastAddr.sin_port = htons(mCastPort);

	/* Join the multicast group */

	mCastInterface.imr_multiaddr.s_addr = inet_addr(mCastIp);
	mCastInterface.imr_interface.s_addr = INADDR_ANY;
	if (setsockopt(mCastSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mCastInterface, sizeof(mCastInterface))) {
		printf("setsockopt() IP_ADD_MEMBERSHIP address %s failed: %d\n", mCastIp, WSAGetLastError());
	}
}

// Receive a frame if one is waiting
void receiveMulticast()
{
	SOCKADDR_IN  stSrcAddr;

	int addr_size = sizeof(struct sockaddr_in);
	if (recvfrom(mCastSocket, buffer, DEFAULT_BUFFER_SIZE, 0, (struct sockaddr*) &stSrcAddr, &addr_size) < 0) {
		printf("recvfrom() failed: %d\n", WSAGetLastError());
		WSACleanup();
		exit(1);
	}
	else {
		printf("Received multicast from %s:%d: %s\n", inet_ntoa(stSrcAddr.sin_addr), ntohs(stSrcAddr.sin_port), buffer);
	}
}

// Sends a multicast to group
void sendMulticast(char msg[])
{
	int result = sendto(mCastSocket, msg, (int)strlen(msg), 0, (struct sockaddr*) &mCastAddr, sizeof(mCastAddr));
	if (result < 0) {
		printf("sendto() failed: %d\n", WSAGetLastError());
		WSACleanup();
		exit(1);
	}
}

void ListenForBroadcast(SOCKET sock)
{
	SetBlocking(sock, false);

	while (1)
	{
		char buffer[DEFAULT_BUFFER_SIZE] = "";
		if (recvfrom(sock, buffer, sizeof(buffer), 0, reinterpret_cast<struct sockaddr*>(&Recv_addr), &recLen))
		{
			if (strcmp(inet_ntoa(Recv_addr.sin_addr), szLocalIP) != 0) {
				string msg = "Received a broadcast packet from ";
				msg.append(inet_ntoa(Recv_addr.sin_addr));
				AddOutputMsg((LPWSTR)msg.c_str);
			}
		}
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
bool initWinsock()
{
    int result = WSAStartup(0x0202, &wsaData); // Winsock 2.2
    if (result != NO_ERROR) {
		AddOutputMsg(L"Encountered error while initializing winsock.");
        return false;
    }

    return true;
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