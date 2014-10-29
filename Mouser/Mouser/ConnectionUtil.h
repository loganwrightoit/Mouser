#pragma once

#include "WinSock2.h"
#pragma comment (lib, "Ws2_32.lib")

bool Send(SOCKET sock, char * inBytes);
bool Receive(SOCKET sock, char * outBytes);

SOCKET GetBcstListenSocket();
bool ListenForBroadcast();
bool SendUdpBroadcast(SOCKET sock);

SOCKET GetMcstListenSocket();
bool InitMulticast();
bool SendMulticast();
bool CloseMulticast();

bool isInit();
bool InitWinsock();
void SetBlocking(SOCKET sock, bool block);
void CloseSocket(SOCKET sock);
SOCKET ConnectTo(char * host, USHORT port, int timeoutSec);