#pragma once

bool Send(SOCKET sock, char * inBytes);
bool Receive(SOCKET sock, char * outBytes);

void SendBroadcast();
void SendMulticast();

bool InitWinsock();
void SetBlocking(SOCKET sock, bool block);
void CloseSocket(SOCKET sock);
SOCKET ConnectTo(char * host, u_short port, int timeoutSec);