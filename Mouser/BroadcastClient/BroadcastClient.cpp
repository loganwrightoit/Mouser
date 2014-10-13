#define PORT 8888   //The port on which to listen for incoming data
#define SERVER "127.0.0.1"  //ip address of udp server

#include <ws2tcpip.h>
#include <string>
#pragma comment (lib, "Ws2_32.lib")

int main(void)
{
	struct sockaddr_in connectedSocket;

	int s;
	int length = sizeof(connectedSocket);

	char receiveBuffer[1000];
	char message[1000];

	//clear the buffer by filling null, it might have previously received data
	memset(receiveBuffer, '\0', 1000);

	WSADATA wsa;
	//Initialise winsock
	printf("\nInitialising Winsock...\n");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("\nFailed. Error Code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	printf("\n.........Initialised.\n");


	//create socket
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == SOCKET_ERROR)
	{
		printf("\n\nsocket() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}


	//setup address structure
	memset((char *)&connectedSocket, 0, sizeof(connectedSocket));
	connectedSocket.sin_family = AF_INET;
	connectedSocket.sin_port = htons(PORT);
	//connectedSocket.sin_port = INADDR_BROADCAST;
	connectedSocket.sin_addr.S_un.S_addr = inet_addr(SERVER);

	printf("\n\n\nEnter message : ");
	gets(message);

	//send the message
	if (sendto(s, message, sizeof(message), 0, (struct sockaddr *) &connectedSocket, sizeof(connectedSocket)) == SOCKET_ERROR)
	{
		printf("\nsendto() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	printf("\nMessage Successfully sent to Server");
	// fflush(stdout);

	if (recvfrom(s, receiveBuffer, 1000, 0, (struct sockaddr *) &connectedSocket, &length) == SOCKET_ERROR)
	{
		printf("\nrecvfrom() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
		while (1);
	}

	printf("\nServer Says : ");
	printf(receiveBuffer, sizeof(receiveBuffer));

	while (1);

	closesocket(s);
	WSACleanup();

	return 0;
}