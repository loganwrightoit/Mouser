#pragma once

#include "WinSock2.h"
#pragma comment (lib, "Ws2_32.lib")

const int DEFAULT_BUFFER_SIZE = 1464;

bool Send(SOCKET sock, char * inBytes, u_int inSize);
u_int GetReceiveLength(SOCKET sock);
bool Receive(SOCKET sock, char * buffer, u_int recvSize);

USHORT GetPrimaryClientPort();
SOCKET GetConnectionSocket();

bool CloseMulticast(SOCKET sock);
SOCKET GetMulticastSocket();
bool SendMulticast(SOCKET sock, char * identifier);
sockaddr_in GetMulticastSenderInfo(SOCKET sock);

bool InitWinsock();
void SetBlocking(SOCKET sock, bool block);