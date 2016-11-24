#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cstring>
#include <iostream>

#define MAX_THREADS_DEFAULT 3
#define SERVER_PORT_DEFAULT 8080
#define PACKET_SIZE_DEFAULT 8

#define SERVER_MODE_NBYTE     0
#define SERVER_MODE_SEPARATOR 1

#pragma comment(lib,"Ws2_32.lib")

using namespace std;

string bufMsg  = "Buffer length: ";
string modeMsg = "Server mode:   ";
string fullMsg = "\nServer is full, connect later o/\n";

int maxThreads;
int serverPort;
int packetSize;
int serverMode;

bool work        = true;
bool serverStart = false;

HANDLE hMutex;
HANDLE serverThread;
SOCKET listenSocket = INVALID_SOCKET;

struct clientDescriptor{
    SOCKET sock;
    char *ip;
    int port;
};

vector<clientDescriptor> clientDesc;

void closeSocket(SOCKET socket){
    WaitForSingleObject(hMutex, INFINITE);
    int killIndex = -1;
    for(int i = 0; i < clientDesc.size(); i++)
        if(clientDesc[i].sock == socket) killIndex = i;
    if(killIndex >= 0){
        string ip = clientDesc[killIndex].ip;
        int port  = clientDesc[killIndex].port;

        clientDesc.erase(clientDesc.begin() + killIndex);
        int temp = closesocket(socket);

        if(temp){
            cout << "Error. Accept socket was not closed" << endl;
        }else{
            cout << ip << ":" << port << " was disconnected" << endl;
        }
    }
    ReleaseMutex(hMutex);
}

int recvN(SOCKET socket, char *buffer){
    char *tempbuf = buffer;
    int cnt = packetSize;
    while(cnt > 0){
        int rc = recv(socket, tempbuf, cnt, 0);
        if(rc <= 0) return 0;
        tempbuf += rc;
        cnt     -= rc;
    }
    return 1;
}

int recvS(SOCKET socket, char *buffer){
    for (int i = 0; i < packetSize; i++) {
        int rc = recv(socket, buffer + i, 1, 0);
        if (rc <= 0) return 0;
        if (buffer[i] == '\n' || buffer[i] == '\0') return 1;
    }
    return 1;
}

int sendMSG(SOCKET socket, string buffer){
    int res = send(socket, buffer.data(), buffer.size(), 0);
    if(res <= 0) return 0;
    return 1;
}

int clientProcess(SOCKET socket){
    int ind;
    bool exitFlag = false;

    for (int i = 0; i < clientDesc.size(); i++){
        if (clientDesc[i].sock == socket) ind = i;
    }

    string Msg = bufMsg + modeMsg;
    sendMSG(socket, Msg);

    char buffer[packetSize + 1];
    memset(&buffer[0], 0, sizeof(buffer));

    while(!exitFlag){
        if(serverMode == SERVER_MODE_NBYTE){
            if(recvN(socket, buffer) == 1)
                cout << clientDesc[ind].ip << ":" << clientDesc[ind].port << " " << buffer << endl;
            else exitFlag = true;

        }else if(serverMode == SERVER_MODE_SEPARATOR){
            if(recvS(socket, buffer) == 1){
                cout << clientDesc[ind].ip << ":" << clientDesc[ind].port << " " << buffer << endl;
                memset(&buffer[0], 0, sizeof(buffer));
            }else exitFlag = true;

        }else exitFlag = true;
    }

    closeSocket(socket);
    return 0;
}

int serverProcess(){
    puts("\nServer process");

    while(true){
        string inputServer;
        cout << "> ";
        cin >> inputServer;

        if(inputServer == "q"){
            cout << "Stopping server..." << endl;
            closesocket(listenSocket);

            work = false;
            return 0;
        }else if(inputServer == "l"){
            if(clientDesc.size() == 0) cout << "Client list is empty" << endl;
            else{
                cout << "Client list:" << endl;
                for(int i = 0; i < clientDesc.size(); i++){
                    cout << clientDesc[i].sock << "|" << clientDesc[i].ip << ":" << clientDesc[i].port << endl;
                }
            }
        }else if(inputServer == "k"){
            SOCKET sock;
            cin >> sock;
            if(cin.fail()) cin.clear();
            else closeSocket(sock);
        }
    }

    return 0;
}

void acceptConnections(SOCKET listenSocket){
    if(!serverStart){
        serverThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)serverProcess, NULL, 0, NULL);
        serverStart  = true;
    }

    clientDescriptor desc;
    sockaddr_in clientInfo;
    int clientInfoSize = sizeof(clientInfo);
    SOCKET acceptSocket;

    while(clientDesc.size() < maxThreads){
        acceptSocket = accept(listenSocket, (struct sockaddr*)&clientInfo, &clientInfoSize);

        if(acceptSocket == INVALID_SOCKET){
                break;
        }

        desc.sock = acceptSocket;
        desc.ip   = inet_ntoa(clientInfo.sin_addr);
        desc.port = clientInfo.sin_port;

        WaitForSingleObject(hMutex, INFINITE);
        clientDesc.push_back(desc);
        ReleaseMutex(hMutex);

        printf("Connection request received.\nNew socket was created at address %s:%d\n", desc.ip, clientInfo.sin_port);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)clientProcess, (void*)acceptSocket, 0, NULL);
    }

    if(clientDesc.size() >= maxThreads){
        acceptSocket = accept(listenSocket, (struct sockaddr*)&clientInfo, &clientInfoSize);

        char *ip  = inet_ntoa(clientInfo.sin_addr);

        sendMSG(acceptSocket, fullMsg);
        closesocket(acceptSocket);

        printf("%s:%d was disconnected because of server overload\n", ip, clientInfo.sin_port);
    }
}

void startWSA(){
    WSADATA wsaDATA;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaDATA);
    if(iResult != 0){
        printf("WSAStartup failed with error: %d\n", iResult);
        exit(1);
    }else
        puts("WSAStartup success");
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

    hMutex = CreateMutex(NULL, false, NULL);
    startWSA();

    struct sockaddr_in server;
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port        = htons(serverPort);
    server.sin_family      = AF_INET;

    if (listenSocket < 0){
        puts("Socket failed with error");
        WSACleanup();
        exit(2);
    }else
        puts("Socket created");

    if(bind(listenSocket, (struct sockaddr *) &server, sizeof(server)) < 0){
        puts("Bind failed. Error");
        exit(3);
    }else
        puts("Bind created");

    if(listen(listenSocket, SOCKET_ERROR) == SOCKET_ERROR){
        puts("Listen call failed. Error");
        exit(4);
    }else
        puts("Listen started");

    while(work)
        acceptConnections(listenSocket);

    return 0;
}