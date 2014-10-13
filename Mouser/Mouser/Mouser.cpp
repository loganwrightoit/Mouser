//#ifndef WIN32_LEAN_AND_MEAN
//#define WIN32_LEAN_AND_MEAN
//#endif

//#include <windows.h>
//#include <winsock2.h>
#include <ws2tcpip.h>

//#include <stdlib.h>
//#include <stdio.h>
#include <string>
#include <iostream>
using namespace std;

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
//#pragma comment (lib, "Mswsock.lib")
//#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 256
#define DEFAULT_PORT "27015"

int __cdecl main(int argc, char **argv)
{
	char *userName = "Logan";
	char serverName[50] = "";

	WSADATA wsaData;
	SOCKET _socket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	char sendbuf[DEFAULT_BUFLEN] = "User has connected!";
	int _result;
	int recvbuflen = DEFAULT_BUFLEN;

	// Validate the parameters
	if (argc != 2) {
		printf("usage: %s server-name\n", argv[0]);
		return 1;
	}

	// Initialize Winsock
	_result = WSAStartup(MAKEWORD(2, 2), &wsaData); // Winsock v2.2
	if (_result != 0) {
		printf("ERROR WSAStartup failed: %d\n", _result);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	_result = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
	if (_result != 0) {
		printf("ERROR: Getaddrinfo failed: %d\n", _result);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		_socket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (_socket == INVALID_SOCKET) {
			printf("ERROR Socket failed: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		_result = connect(_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (_result == SOCKET_ERROR) {
			closesocket(_socket);
			_socket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (_socket == INVALID_SOCKET) {
		printf("ERROR Unable to connect to server\n");
		WSACleanup();
		return 1;
	}

	// Send messages until user closes connection
	do {
		_result = send(_socket, sendbuf, (int)strlen(sendbuf), 0);
		if (_result == SOCKET_ERROR) {
			printf("ERROR Send failed: %d\n", WSAGetLastError());
			closesocket(_socket);
			WSACleanup();
			return 1;
		}

		cout << userName << ": ";
		cin.getline(sendbuf, DEFAULT_BUFLEN);

	} while (strcmp(sendbuf, "close") != 0);

	// shutdown the connection since no more data will be sent
	_result = shutdown(_socket, SD_SEND);
	if (_result == SOCKET_ERROR) {
		printf("ERROR Shutdown failed: %d\n", WSAGetLastError());
		closesocket(_socket);
		WSACleanup();
		return 1;
	}

	char *recvbuf = "buffer";
	// Receive until the peer closes the connection
	do {

		_result = recv(_socket, recvbuf, recvbuflen, 0);
		if (_result > 0)
			printf("Bytes received: %d\n", _result);
		else if (_result == 0)
			printf("Connection closed\n");
		else
			printf("ERROR Recieve failed: %d\n", WSAGetLastError());

	} while (_result > 0);

	// cleanup
	closesocket(_socket);
	WSACleanup();

	return 0;
}