#include "stdafx.h"
#include "Peer.h"

Peer::Peer(SOCKET inSock)
{
    sock = inSock;
}

Peer::~Peer()
{
    Disconnect();
}

void Peer::SendChatMsg(char * inBytes)
{
    Send(sock, inBytes, strlen(inBytes));
}

void Peer::Disconnect()
{
    shutdown(sock, SD_SEND);
    closesocket(sock);
}