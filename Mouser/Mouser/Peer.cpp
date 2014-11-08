#include "stdafx.h"
#include "Peer.h"

Peer::Peer(SOCKET sock, char * identifier)
{
    this->sock = sock;
    this->identifier = identifier;
}

Peer::~Peer()
{
    Disconnect();
}

void Peer::SendString(char * inBytes)
{
    Send(sock, inBytes, strlen(inBytes));
}

void Peer::SendCharArray(char * inBytes, u_int inLength)
{
    Send(sock, inBytes, inLength);
}

void Peer::Disconnect()
{
    shutdown(sock, SD_BOTH);
    closesocket(sock);
}