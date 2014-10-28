#include "stdafx.h"
#include <iostream>
#include <ws2tcpip.h>
#include "ConnectionModule.h"

using namespace std;

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 256

WSADATA wsaData;
struct sockaddr_in clientService;

void CloseSocket(SOCKET sock)
{
    shutdown(sock, SD_SEND);
    closesocket(sock);
}

//
// Transmit message over connection.
// Returns true if message successfully sent.
//
bool Send(SOCKET sock, CHAR * inBytes)
{
    // Prepend message with two-byte length header
    u_short len = strlen(inBytes);
    byte byteLen[] = { (len & 0xff00) >> 8, (len & 0xff) };
    string s = "";
    s += byteLen[0];
    s += byteLen[1];
    s += inBytes;

    // Transmit remaining bytes
    int result = send(sock, s.c_str(), s.length(), 0);
    if (result == SOCKET_ERROR)
    {
        cout << "DEBUG: Error requesting: " << WSAGetLastError() << ", shutting down connection." << endl;
        CloseSocket(sock);
        return false;
    }
    else
    {
        return true;
    }
}

//
// Receive transaction data and fill outBytes.
//
bool Receive(SOCKET sock, char * outBytes)
{
    string s = "";

    // Receive first two bytes (message length)
    u_short messageLength;
    int inBytes = recv(sock, (char*)&messageLength, sizeof(messageLength), 0);

    if (inBytes > 0) // Receive all the data
    {
        int bytesLeft = inBytes;
        do {
            // Receive all the data
            char buffer[DEFAULT_BUFLEN] = "";
            bytesLeft -= recv(sock, (char*)&buffer, sizeof(buffer), 0);
            s.append(buffer);
        } while (bytesLeft > 0);

        strcpy(outBytes, const_cast<char*>(s.c_str()));
        return true;
    }
    else if (inBytes != WSAEWOULDBLOCK && inBytes != 0) // Nothing to receive, don't block
    {
        cout << "DEBUG: Error receiving: " << WSAGetLastError() << ", shutting down connection." << endl;
        CloseSocket(sock);
    }

    return false;
}

//
// Initializes Winsock
//
bool initWinsock()
{
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData); // Winsock 2.2
    if (result != NO_ERROR) {
        cout << "ERROR starting winsock: " << result << endl;
        return false;
    }

    return true;
}

//
// Sets blocking mode of socket.
// TRUE blocks, FALSE is non-blocking
//
void SetBlocking(SOCKET sock, bool block)
{
    block = !block;
    unsigned long mode = block;
    int result = ioctlsocket(sock, FIONBIO, &mode);
    if (result != NO_ERROR)
    {
        cout << "ERORR failed setting socket blocking mode: " << result << endl;
    }
}

//
// Opens a client connection to server.
//  Params:
//      host - IP address of server
//      port - Port of server
//      timeoutSec - Timeout for connection attempt in seconds
//  Return:
//      a SOCKET pointer
//
SOCKET ConnectTo(char * host, u_short port, int timeoutSec)
{
    TIMEVAL Timeout;
    Timeout.tv_sec = timeoutSec;
    Timeout.tv_usec = 0;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in address;
    address.sin_addr.s_addr = inet_addr(host);
    address.sin_port = htons(port);
    address.sin_family = AF_INET;

    // Set to non-blocking and start a connection attempt

    SetBlocking(sock, false);
    connect(sock, (struct sockaddr *)&address, sizeof(address));

    fd_set Write, Err;
    FD_ZERO(&Write);
    FD_ZERO(&Err);
    FD_SET(sock, &Write);
    FD_SET(sock, &Err);

    // Set back to blocking and check if socket is ready after timeout

    SetBlocking(sock, true);
    select(0, NULL, &Write, &Err, &Timeout);
    if (FD_ISSET(sock, &Write))
    {
        SetBlocking(sock, false);
        return sock;
    }
    else {
        return INVALID_SOCKET;
    }
}