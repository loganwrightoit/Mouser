#pragma once

class Peer
{
public:

    Peer(SOCKET sock, char * identifier);
    ~Peer();

    void SendString(char * inBytes);
    void SendCharArray(char * inBytes, u_int inLength);
    void Disconnect();

private:

    char * identifier;
    SOCKET sock;

};

