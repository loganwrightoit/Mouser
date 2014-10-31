#pragma once

#include "WinSock2.h"
#pragma comment (lib, "Ws2_32.lib")

bool Send(SOCKET sock, char * inBytes);
bool Receive(SOCKET sock, char * outBytes);

SOCKET GetConnectionSocket();

USHORT GetPrimaryClientPort();

SOCKET GetBroadcastSocket();
bool SendBroadcast(SOCKET sock);
sockaddr_in ReceiveBroadcast(SOCKET sock);

bool CloseMulticast(SOCKET sock);
SOCKET GetMulticastSocket();
bool SendMulticast(SOCKET sock);
sockaddr_in ReceiveMulticast(SOCKET sock);
bool SetMulticastTTL(SOCKET sock, int TTL);

bool InitWinsock();
void SetBlocking(SOCKET sock, bool block);
void CloseSocket(SOCKET sock);
SOCKET ConnectTo(char * host, USHORT port, int timeoutSec);