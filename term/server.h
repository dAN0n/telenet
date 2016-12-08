#pragma once
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cstring>
#include <iostream>
#include "term.h"

#define MAX_THREADS_POSSIBLE 10
#define MAX_THREADS_DEFAULT 2
#define SERVER_PORT_DEFAULT 8080
#define PACKET_SIZE_DEFAULT 8

#pragma comment(lib,"Ws2_32.lib")

using namespace std;

int main(int argc, char *argv[]);
int recvS(SOCKET socket, char *buf, string &line);
int sendMSG(SOCKET socket, string buffer);
DWORD WINAPI clientProcess(void* socket);
DWORD WINAPI serverProcess();
DWORD WINAPI acceptConnections(void *listenSocket);
void startWSA();
void closeSocket(int ind);