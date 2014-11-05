#pragma once

#include "WinSock2.h"
#pragma comment (lib, "Ws2_32.lib")

bool Send(SOCKET sock, char * inBytes, u_int inSize);
u_int GetReceiveLength(SOCKET sock);
bool Receive(SOCKET sock, char * buffer, u_int recvSize);

char * GetMyHost();
USHORT GetPrimaryClientPort();

SOCKET GetConnectionSocket();

SOCKET GetBroadcastSocket();
bool SendBroadcast(SOCKET sock);
sockaddr_in GetBroadcastSenderInfo(SOCKET sock);

bool CloseMulticast(SOCKET sock);
SOCKET GetMulticastSocket();
bool SendMulticast(SOCKET sock);
sockaddr_in GetMulticastSenderInfo(SOCKET sock);

bool InitWinsock();
void SetBlocking(SOCKET sock, bool block);