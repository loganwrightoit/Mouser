#pragma once

#include "WinSock2.h"
#pragma comment (lib, "Ws2_32.lib")

bool Send(SOCKET sock, char * inBytes);
bool Receive(SOCKET sock, char * outBytes);

SOCKET GetBroadcastSocket();
bool SendBroadcast(SOCKET sock);
bool ReceiveBroadcast(SOCKET sock);

SOCKET GetMulticastSocket(int TTL);
bool SendMulticast(SOCKET sock);
bool ReceiveMulticast(SOCKET sock);

bool InitWinsock();
void SetBlocking(SOCKET sock, bool block);
void CloseSocket(SOCKET sock);
SOCKET ConnectTo(char * host, USHORT port, int timeoutSec);