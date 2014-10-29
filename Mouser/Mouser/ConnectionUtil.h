#pragma once

bool Send(SOCKET sock, char * inBytes);
bool Receive(SOCKET sock, char * outBytes);

bool SendBroadcast();

bool InitMulticast();
bool SendMulticast();
bool CloseMulticast();

void SetBlocking(SOCKET sock, bool block);
void CloseSocket(SOCKET sock);
SOCKET ConnectTo(char * host, USHORT port, int timeoutSec);