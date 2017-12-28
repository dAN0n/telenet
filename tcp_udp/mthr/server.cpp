#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cstring>
#include <iostream>

#define MAX_THREADS_POSSIBLE 10
#define MAX_THREADS_DEFAULT 2
#define SERVER_PORT_DEFAULT 8080
#define PACKET_SIZE_DEFAULT 8

#define SERVER_MODE_NBYTE     0
#define SERVER_MODE_SEPARATOR 1

#pragma comment(lib,"Ws2_32.lib")

using namespace std;

string bufMsg  = "Buffer length: ";
string modeMsg = "Server mode:   ";
string fullMsg = "\nServer is full, connect later o/\n";

struct clientDescriptor{
    SOCKET sock;
    HANDLE handle;
    char *ip;
    int port;
};

int maxThreads;
int serverPort;
int packetSize;
int serverMode;

clientDescriptor clientDesc[MAX_THREADS_POSSIBLE];
SOCKET listenSocket = INVALID_SOCKET;
const HANDLE hMutex = CreateMutex(NULL, false, NULL);

void closeSocket(int ind){
    WaitForSingleObject(hMutex, INFINITE);
    if(ind >= 0 && ind < maxThreads && clientDesc[ind].sock != INVALID_SOCKET){
        // if(shutdown(clientDesc[ind].sock, SD_BOTH))
            // puts("Shutdown failed");
        // else{
            if(closesocket(clientDesc[ind].sock))
                puts("Closesocket failed");
            else{
                cout << clientDesc[ind].ip << ":" << clientDesc[ind].port << " was disconnected" << endl;
                clientDesc[ind].sock = INVALID_SOCKET;
            }
        // }
    }
    ReleaseMutex(hMutex);
}

int recvN(SOCKET socket, char *buffer){
    int cnt = packetSize;
    while(cnt > 0){
        int rc = recv(socket, buffer, cnt, 0);
        if(rc <= 0) return 0;
        buffer += rc;
        cnt    -= rc;
    }
    return 1;
}

int recvS(SOCKET socket, char *buffer){
    for(int i = 0; i < packetSize; i++) {
        if(recv(socket, buffer + i, 1, 0) <= 0) return 0;
        if (buffer[i] == '\n' || buffer[i] == '\0') return 1;
    }
    return 1;
}

int sendMSG(SOCKET socket, string buffer){
    if(send(socket, buffer.data(), buffer.size(), 0) <= 0) return 0;
    return 1;
}

DWORD WINAPI clientProcess(void* socket){
    int ind;
    int rc = 1;

    for (int i = 0; i < maxThreads; i++){
        if (clientDesc[i].sock == (SOCKET)socket) ind = i;
    }

    string Msg = bufMsg + modeMsg;
    sendMSG(clientDesc[ind].sock, Msg);

    char buffer[packetSize + 1];
    memset(&buffer[0], 0, sizeof(buffer));

    while(rc != 0){
        if(serverMode == SERVER_MODE_NBYTE)
            rc = recvN(clientDesc[ind].sock, buffer);
        else if(serverMode == SERVER_MODE_SEPARATOR)
            rc = recvS(clientDesc[ind].sock, buffer);
        else break;

        if(rc != 0){
            cout << clientDesc[ind].ip << ":" << clientDesc[ind].port << " " << buffer << endl;
            memset(&buffer[0], 0, sizeof(buffer));
        }

    }

    closeSocket(ind);
    return 0;
}

DWORD WINAPI serverProcess(){
    while(true){
        string inputServer;
        cin >> inputServer;

        if(inputServer == "q"){
            puts("Stopping server...");
            shutdown(listenSocket, SD_BOTH);
            closesocket(listenSocket);

            return 0;
        }else if(inputServer == "l"){
            puts("Client list:");

            WaitForSingleObject(hMutex, INFINITE);
            int lineCount = 0;
            for(int i = 0; i < maxThreads; i++){
                if(clientDesc[i].sock != INVALID_SOCKET){
                    cout << i << "|" << clientDesc[i].ip << ":" << clientDesc[i].port << endl;
                    lineCount++;
                }
            }
            if(lineCount == 0) puts("empty");
            ReleaseMutex(hMutex);
        }else if(inputServer == "k"){
            SOCKET sock;
            cin >> sock;
            if(cin.fail()) cin.clear();
            else closeSocket(sock);
        }
    }

    return 0;
}

DWORD WINAPI acceptConnections(void *listenSocket){
    SOCKET acceptSocket;
    clientDescriptor desc;
    sockaddr_in clientInfo;
    int clientInfoSize = sizeof(clientInfo);

    while(true){
        int ind = -1;

        acceptSocket = accept((SOCKET)listenSocket, (struct sockaddr*)&clientInfo, &clientInfoSize);

        if(acceptSocket == INVALID_SOCKET) break;
        else if(acceptSocket < 0){
            puts("Accept failed");
            continue;
        }

        WaitForSingleObject(hMutex, INFINITE);
        for (int i = maxThreads - 1; i >= 0; i--)
        if(clientDesc[i].sock == INVALID_SOCKET){
            ind = i;
        }
        ReleaseMutex(hMutex);

        if(ind < 0){
            char *ip  = inet_ntoa(clientInfo.sin_addr);

            sendMSG(acceptSocket, fullMsg);
            shutdown(acceptSocket, SD_BOTH);
            closesocket(acceptSocket);

            cout << ip << ":" << clientInfo.sin_port << " was disconnected because of server overload" << endl;
            continue;
        }

        desc.sock = acceptSocket;
        desc.ip   = inet_ntoa(clientInfo.sin_addr);
        desc.port = clientInfo.sin_port;

        cout << "Connection request received." << endl;
        cout << "New socket was created at address " << desc.ip << ":" << desc.port << endl;

        clientDesc[ind] = desc;
        clientDesc[ind].handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)clientProcess, (void *)desc.sock, 0, NULL);
    }

    WaitForSingleObject(hMutex, INFINITE);
    vector<HANDLE> hClients;
    for(int i = 0; i < maxThreads; i++)
        if(clientDesc[i].sock != INVALID_SOCKET){
            hClients.push_back(clientDesc[i].handle);
            closeSocket(i);
        }
    ReleaseMutex(hMutex);

    if (!hClients.empty()){
        WaitForMultipleObjects(hClients.size(), hClients.data(), TRUE, INFINITE);
    }
    return 0;
}

void startWSA(){
    WSADATA wsaDATA;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaDATA);
    if(iResult != 0){
        printf("WSAStartup failed with error: %d\n", iResult);
        exit(1);
    }
}

int main(int argc, char *argv[]){

    if(argc > 1){
        for(int i = 1; i < argc; i++){
            string opt = string(argv[i]);

            if(opt == "-p" || opt == "--port"){
                if(i < argc - 1){
                    int arg = atoi(argv[i + 1]);
                    if(arg > 0 && arg < 65536) serverPort = arg;
                }
            }
            else if(opt == "-s" || opt == "--separator"){
                serverMode = SERVER_MODE_SEPARATOR;
            }
            else if(opt == "-b" || opt == "--buffer"){
                if(i < argc - 1){ 
                    int arg = atoi(argv[i + 1]);
                    if(arg > 0 && arg < 10) packetSize = arg;
                }
            }
            else if(opt == "-t" || opt == "--threads"){
                if(i < argc - 1){
                    int arg = atoi(argv[i + 1]);
                    if(arg > 0 && arg < 11) maxThreads = arg;
                }
            }
            else if(opt == "-h" || opt == "--help"){
                cout << endl;
                cout << "OPTIONS"                                                   << endl;
                cout << "-b, --buffer [1-9]         Buffer length, default: 8"      << endl;
                cout << "-h, --help                 Show this message and close"    << endl;
                cout << "-p, --port [1-65535]       Listen port, default: 8080"     << endl;
                cout << "-s, --separator            Messages till separator"        << endl;
                cout << "-t, --threads [1-10]       Maximum threads, default: 3"    << endl << endl;
                cout << "SERVER OPTIONS"                                            << endl;
                cout << "l                          List all online clients"        << endl;
                cout << "k [number]                 Kill client"                    << endl;
                cout << "q                          Server shutdown"                << endl;
                return(0);
            }
        }
    }

    if(maxThreads == 0) maxThreads = MAX_THREADS_DEFAULT;
    if(serverPort == 0) serverPort = SERVER_PORT_DEFAULT;
    if(packetSize == 0) packetSize = PACKET_SIZE_DEFAULT;
    if(serverMode == 0) serverMode = SERVER_MODE_NBYTE;

    char temp[2];
    bufMsg  = bufMsg  + itoa(packetSize, temp, 10) + "\n";
    modeMsg = modeMsg + itoa(serverMode, temp, 10) + "\n";

    cout << "Server settings"         << endl;
    cout << "Threads: " << maxThreads << endl;
    cout << "Port:    " << serverPort << endl;
    cout << "Buffer:  " << packetSize << endl;
    cout << "Mode:    " << serverMode << endl << endl;

    startWSA();

    WaitForSingleObject(hMutex, INFINITE);
    for (int i = 0; i < maxThreads; i++)
        clientDesc[i].sock = INVALID_SOCKET;
    ReleaseMutex(hMutex);

    sockaddr_in server;
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port        = htons(serverPort);
    server.sin_family      = AF_INET;

    if (listenSocket < 0){
        puts("Socket failed with error");
        WSACleanup();
        exit(2);
    }

    if(bind(listenSocket, (struct sockaddr *) &server, sizeof(server)) < 0){
        puts("Bind failed. Error");
        exit(3);
    }

    if(listen(listenSocket, SOCKET_ERROR) == SOCKET_ERROR){
        puts("Listen call failed. Error");
        exit(4);
    }

    HANDLE hAccept = CreateThread(NULL, 0, acceptConnections, (void *)listenSocket, 0, NULL);

    serverProcess();

    WaitForSingleObject(hAccept, INFINITE);

    WSACleanup();
    return 0;
}