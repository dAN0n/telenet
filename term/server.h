#include <winsock2.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <conio.h>
#include <clocale>

#define MAX_THREADS_POSSIBLE 10
#define MAX_THREADS_DEFAULT 2
#define SERVER_PORT_DEFAULT 8080
#define PACKET_SIZE_DEFAULT 8

#pragma comment(lib,"Ws2_32.lib")

using namespace std;

/***************
SERVER FUNCTIONS
***************/

int main(int argc, char *argv[]);
int recvS(SOCKET socket, char *buf, string &line);
int sendMSG(SOCKET socket, string buffer);
DWORD WINAPI clientProcess(void* socket);
DWORD WINAPI serverProcess();
DWORD WINAPI acceptConnections(void *listenSocket);
void startWSA();
void closeSocket(int ind);

/*****************
TERMINAL FUNCTIONS
*****************/

int terminal();
int rewriteUserFile();
int getUserIndex(string login);
int readUserFile();
int addusrCommand(string login, string password);
int loginCommand(string login, string password);
int chmodCommand(string opt);
int killCommand(string login);
string getServerPath();
string cdCommand(SOCKET sock, string dir, string path);
vector<string> lsCommand(string folder);
bool compareDir(string i, string j);