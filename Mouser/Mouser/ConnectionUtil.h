#pragma once

#include "WinSock2.h"
#pragma comment (lib, "Ws2_32.lib")

const int DEFAULT_BUFFER_SIZE = 1464;

bool Send(SOCKET sock, char * inBytes, u_int inSize);
u_int GetReceiveLength(SOCKET sock);
bool Receive(SOCKET sock, char * buffer, u_int recvSize);

USHORT GetPrimaryClientPort();
SOCKET GetConnectionSocket();

bool CloseMulticast();
SOCKET GetMulticastSocket();
bool SendMulticast(char * identifier);
sockaddr_in GetMulticastSenderInfo();

bool InitWinsock();
void SetBlocking(SOCKET sock, bool block);

void SetupMulticastListener(HWND hWnd);
void SetupConnectionListener(HWND hWnd);