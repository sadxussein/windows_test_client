#define WIN32_LEAN_AND_MEAN	// it is necessary for resolving conflict between socket versions in <Windows.h> and <winsock2.h>

#include <Windows.h>	// master include file for windows apps
#include <winsock2.h>	
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")	// must be referenced to particular lib, app wont work otherwise
#pragma comment (lib, "Mswsock.lib")	// used by winsock 2
#pragma comment (lib, "AdvApi32.lib")

int __cdecl main(int argc, char *argv[]) {	// cdecl/_cdecl/__cdecl is some stack cleanup bollocks, not important atm
	WSADATA wsaData;	// here we storage details about winsock implementations
	SOCKET ConnectSocket = INVALID_SOCKET;	
	struct addrinfo* result = NULL,	// addrinfo contains sockaddr
		* ptr = NULL,
		hints;
	int iResult;	// function result handler
	char buffer[1500];	// set buffer size to default MTU
	const char* initial_message = "You have me now.";	// initial message sent to server

	if (argc < 3) {	// user should provide the app with 2 arguments
		printf("usage: %s server-address server-port", argv[0]);
		return 1;
	}

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);	// calling WS2_32.dll lib; initializing winsock 2.2
	if (iResult != 0) {
		printf("WSAStartup failed with error code: %d\n", iResult);
		return 1;
	}	
		
	ZeroMemory(&hints, sizeof(hints));	// initializing with zeroes
	hints.ai_family = AF_INET;	// we want ipv4 internet domain for our server-client model
	hints.ai_socktype = SOCK_STREAM;	// tcp
	hints.ai_protocol = 0;	// linux-like, let system decide which protocol to choose, it can be IPPROTO_TCP

	iResult = getaddrinfo(argv[1], argv[2], &hints, &result);	// resolving server address and port; result is where we store server (response) connection info, it could be multiple
	if (iResult != 0) {
		printf("getaddrinfo failed with error code: %d\n", iResult);
		WSACleanup();	// if something goes south - clean the mess, end the program, terminate connection to WS2_32.dll
		return 1;
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);	// creating a socket using first address returned by getaddrinfo()
		if (ConnectSocket == INVALID_SOCKET) {	// if socket() failes
			printf("socket() failed with error code: %d\n", WSAGetLastError());
			WSACleanup();	// if something goes south - clean the mess, end the program, terminate connection to WS2_32.dll
			return 1;
		}

		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);	// attempting to connect to server
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;	// if we get bad socket, attempt another, since result can get multiple answers from server
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {	// if socket is still invalid, clean up the mess, close the program
		printf("unable to connect to server!\n");
		WSACleanup();	// if something goes south - clean the mess, end the program, terminate connection to WS2_32.dll
		return 1;
	}
	
	iResult = send(ConnectSocket, initial_message, (int)strlen(initial_message), 0);	// send to server an initial message
	if (iResult == SOCKET_ERROR) {
		printf("send() failed with error code: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();	// if something goes south - clean the mess, end the program, terminate connection to WS2_32.dll
		return 1;
	}

	printf("bytes sent by client to server: %ld\n", iResult);

	iResult = shutdown(ConnectSocket, SD_SEND);	// closing ONLY outgoing connection, we can still receive bytes from server
	if (iResult == SOCKET_ERROR) {
		printf("shutdown() failed with error code: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();	// if something goes south - clean the mess, end the program, terminate connection to WS2_32.dll
		return 1;
	}

	do {
		iResult = recv(ConnectSocket, buffer, 1500, 0);	// while server is working with us we recieve bytes of information to a buffer
		if (iResult > 0) printf("bytes recieved by server: %d\n", iResult);	// still working
		else if (iResult == 0) printf("connection terminated\n");	// connection over
		else printf("recv() failed with error code: %d\n", WSAGetLastError());	// error of some sort
	} while (iResult > 0);

	closesocket(ConnectSocket);	// clean up after work is finished
	WSACleanup();

	return 0;
}