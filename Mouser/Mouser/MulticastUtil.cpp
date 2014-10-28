#include "stdafx.h"
#include "MulticastUtil.h"

MulticastUtil::MulticastUtil()
{
}

MulticastUtil::~MulticastUtil()
{
}

///////// OLD CODE

//#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#pragma comment (lib, "ws2_32.lib")

#define BUFSIZE     1024
#define MCAST_PORT  41921
#define MCAST_IP    "234.241.92.163"
#define MCAST_TTL   2 // Set higher to traverse routers

char           buffer[BUFSIZE];
struct ip_mreq mCastInterface;
SOCKET         mCastSocket;
char           mCastIp[] = MCAST_IP;
u_short        mCastPort = MCAST_PORT;
u_long         mCastTTL = MCAST_TTL;
SOCKADDR_IN    mCastAddr, clientAddr;

void printBanner(void)
{
    printf("------------------------------------------------------\n");
    printf("                Mouser Multicast Test                 \n");
    printf("------------------------------------------------------\n");
}

// Initializes multicast
void initMulticast()
{
    BOOL flag;
    int result;
    WSADATA stWsaData;

    /* Initialize winsock v2.2 */

    result = WSAStartup(0x0202, &stWsaData);
    if (result) {
        printf("WSAStartup failed: %d\r\n", result);
        exit(1);
    }

    /* Assign socket */

    mCastSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (mCastSocket == INVALID_SOCKET) {
        printf("socket() failed: %d\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }

    /* Bind socket */

    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    clientAddr.sin_port = htons(mCastPort);
    if (bind(mCastSocket, (struct sockaddr*) &clientAddr, sizeof(clientAddr))) {
        printf("bind() port: %d failed: %d\n", mCastPort, WSAGetLastError());
    }

    /* Set time-to-live if not default size */

    if (mCastTTL > 1) {
        if (setsockopt(mCastSocket, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&mCastTTL, sizeof(mCastTTL))) {
            printf("setsockopt() IP_MULTICAST_TTL failed: %d\n", WSAGetLastError());
        }
    }

    /* Disable loopback */

    flag = FALSE;
    if (setsockopt(mCastSocket, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&flag, sizeof(flag))) {
        printf("setsockopt() IP_MULTICAST_LOOP failed: %d\n", WSAGetLastError());
    }

    /* Assign destination address */

    mCastAddr.sin_family = AF_INET;
    mCastAddr.sin_addr.s_addr = inet_addr(mCastIp);
    mCastAddr.sin_port = htons(mCastPort);

    /* Join the multicast group */

    mCastInterface.imr_multiaddr.s_addr = inet_addr(mCastIp);
    mCastInterface.imr_interface.s_addr = INADDR_ANY;
    if (setsockopt(mCastSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mCastInterface, sizeof(mCastInterface))) {
        printf("setsockopt() IP_ADD_MEMBERSHIP address %s failed: %d\n", mCastIp, WSAGetLastError());
    }
}

// Receive a frame if one is waiting
void receiveMulticast()
{
    SOCKADDR_IN  stSrcAddr;

    int addr_size = sizeof(struct sockaddr_in);
    if (recvfrom(mCastSocket, buffer, BUFSIZE, 0, (struct sockaddr*) &stSrcAddr, &addr_size) < 0) {
        printf("recvfrom() failed: %d\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }
    else {
        printf("Received multicast from %s:%d: %s\n", inet_ntoa(stSrcAddr.sin_addr), ntohs(stSrcAddr.sin_port), buffer);
    }
}

// Sends a multicast to group
void sendMulticast(char msg[])
{
    int result = sendto(mCastSocket, msg, (int)strlen(msg), 0, (struct sockaddr*) &mCastAddr, sizeof(mCastAddr));
    if (result < 0) {
        printf("sendto() failed: %d\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    printBanner();
    initMulticast();

    sendMulticast("I've joined the multicast!"); // Notify other clients on network that we are available

    /* Begin listening for multicasts */

    printf("Listening for multicast...\n");
    while (1) {
        receiveMulticast();
        Sleep(5000); // Wait 5 seconds before trying again.
    }

    /* Close everything */

    if (setsockopt(mCastSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mCastInterface, sizeof(mCastInterface))) {
        printf("setsockopt() IP_DROP_MEMBERSHIP address %s failed: %d\n", mCastIp, WSAGetLastError());
    }

    closesocket(mCastSocket);
    WSACleanup();

    return (0);
}
