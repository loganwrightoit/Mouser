#include "stdafx.h"
#include "BroadcastUtil.h"

/////////// OLD CODE

#include <iostream>
#include <conio.h>
#include <WinSock2.h>
#include <ctime>
#include <process.h>
#include <ws2tcpip.h>

#pragma comment (lib, "ws2_32.lib")
using namespace std;

BroadcastUtil::BroadcastUtil()
{
}

BroadcastUtil::~BroadcastUtil()
{
}

#define PORT 7777

char currentTime[16];

SOCKET sock;
char recvBuff[50];
int recvBuffLen = 50;
struct sockaddr_in Recv_addr;
struct sockaddr_in Sender_addr;
socklen_t recLen = sizeof(Recv_addr);
struct tm now;
char * szLocalIP;

void getTime()
{
    time_t t = time(0); // get time now
    localtime_s(&now, &t);
    strftime(currentTime, 80, "%H:%M:%S", &now);
}

void recvFunct(void *param)
{
    while (1)
    {
        if (recvfrom(sock, recvBuff, sizeof(recvBuff), 0, reinterpret_cast<struct sockaddr*>(&Recv_addr), &recLen))
        {
            if (strcmp(inet_ntoa(Recv_addr.sin_addr), szLocalIP) != 0) {
                cout << "Processed recvfrom : " << inet_ntoa(Recv_addr.sin_addr) << recvBuff << '\n';
            }
        }
    }
}

void sendFunct1(void *param)
{
    while (1)
    {
        getTime();
        if (sendto(sock, currentTime, strlen(currentTime) + 1, 0, (sockaddr *)&Sender_addr, sizeof(Sender_addr)) < 0)
        {
            cout << endl << "ERROR: send failed." << endl;
            _getch();
            closesocket(sock);
        }
        Sleep(3000);
    }
}

// Cleans up connections
void cleanUp()
{
    closesocket(sock);
    WSACleanup();
}

int main()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    char broadcast = 'a';
    int _result = 0;

    _result = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    if (_result != 0) {
        printf("ERROR: setting socket failed: %d\n", _result);
        cleanUp();
        return 1;
    }

    /* Resolve machine IP address */

    char szHostName[255];
    gethostname(szHostName, 255);
    struct hostent *host_entry = gethostbyname(szHostName);
    szLocalIP = inet_ntoa(*(struct in_addr *)*host_entry->h_addr_list);

    /* Setup structs */

    Recv_addr.sin_family = AF_INET;
    Recv_addr.sin_port = htons(PORT); // Handle endianness
    Recv_addr.sin_addr.s_addr = INADDR_ANY;

    Sender_addr.sin_family = AF_INET;
    Sender_addr.sin_port = htons(PORT); // Handle endianness
    Sender_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    if (bind(sock, (sockaddr*)&Recv_addr, sizeof(Recv_addr)) < 0)
    {
        perror("bind");
        _getch();
        closesocket(sock);
        return 1;
    }

    _beginthread(recvFunct, 0, NULL);
    _beginthread(sendFunct1, 0, NULL);

    cout << "spawned threads, press any key to exit.. \n";
    _getch();
    closesocket(sock);
    WSACleanup();
    return 0;
}