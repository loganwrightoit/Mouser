#include "Peer.h"
#include "ConnectionUtil.h"

Peer::Peer(SOCKET sock)
{
}

Peer::~Peer()
{
    Disconnect();
}

void Peer::SendChatMsg(char * msg)
{
    Send(sock, msg);
}

void Peer::Disconnect()
{
    shutdown(sock, SD_SEND);
    closesocket(sock);
}