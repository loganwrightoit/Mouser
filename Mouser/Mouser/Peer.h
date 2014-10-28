#pragma once
#include <ws2tcpip.h>
#include "ConnectionUtil.h"

class Peer
{
public:

    Peer(SOCKET sock);
    ~Peer();

    void SendChatMsg(char * msg);
    void Disconnect();

private:

    char * name;
    SOCKET sock;

};

