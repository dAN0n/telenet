#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cstring>
#include <iostream>

#define MAX_THREADS_DEFAULT 3
#define SERVER_PORT_DEFAULT 8080
#define PACKET_SIZE_DEAFULT 8

#define SERVER_MODE_NBYTE 0
#define SERVER_MODE_SEPARATOR 1

#pragma comment(lib,"Ws2_32.lib")

using namespace std;

int maxThreads;
int serverPort;
int packetSize;
int serverMode;

string bufMsg = "Buffer length: ";
string modeMsg = "Server mode:   ";

HANDLE hMutex;
int currentThreads = 0;
bool readn;
bool work = true;
bool serverStart = false;

vector<int> clientId;
vector<string> clientIp;
vector<u_short> clientPort;
HANDLE serverThread;

SOCKET listenSocket = INVALID_SOCKET;

struct ArgsThread{
    void *threadData;
    string ip;
    u_short port;
};

void closeSocket(SOCKET sock){
    WaitForSingleObject(hMutex, INFINITE);
    int kill_index = -1;
    for(int i = 0; i < clientId.size(); i++)
        if(clientId[i] == sock)
            kill_index = i;
    if(kill_index >= 0){
        string ip = clientIp[kill_index];
        u_short port = clientPort[kill_index];
        clientId.erase(clientId.begin() + kill_index);
        clientIp.erase(clientIp.begin() + kill_index);
        clientPort.erase(clientPort.begin() + kill_index);

        int temp = closesocket(sock);
        if(temp){
            cout << "Error. Accept socket was not closed" << endl;
        }else{
            cout << ip << ":" << port << " disconnected" << endl;
            currentThreads--;
        }
    }
    ReleaseMutex(hMutex);
}

int readS(SOCKET socket, char *buffer, int len){
    int cnt, rc;
    cnt = len;
    while(cnt > 0){
        rc = recv(socket, buffer, cnt, 0);
        if(rc <= 0){
            //puts("Recv call failed. Error");
            return -1;
        }
        buffer += rc;
        cnt -= rc;
    }
    return len;
}

int readN(SOCKET socket, char *buffer, int len, string &rez){
    int k = readS(socket, buffer, len);
    if(k < 0){
        //puts("Recv call failed. Error");
        return 0;
    }
    for(int i = 0; i < len; i++)
        rez = rez + buffer[i];
    return 1;
}

int readTillSeparator(SOCKET socket, char *buffer, string& rez){
    int rc = 1;
    while(rc != 0){

        rc = recv(socket, buffer, 1, 0);
        if(rc <= 0){
            //puts("Recv call failed. Error");
            return 0;
        }
        if(buffer[0] == '\n')
            return 1;
        rez = rez + buffer[0];
    }
}

int sendMSG(SOCKET socket, char* buffer){
    int res = send(socket, buffer, strlen(buffer), 0);
    if(res <= 0)
        return 0;
    return 1;
}

int clientProcess(ArgsThread *arg){
    SOCKET mySocket = (SOCKET)arg->threadData;
    WaitForSingleObject(hMutex, INFINITE);
    clientId.insert(clientId.end(), mySocket);
    clientIp.insert(clientIp.end(), arg->ip);
    clientPort.insert(clientPort.end(), arg->port);
    ReleaseMutex(hMutex);
    bool exitFlag = false;
    string rez = "";

    char *bufMsgChar = new char[bufMsg.length() + 1];
    char *modeMsgChar = new char[modeMsg.length() + 1];
    strcpy(bufMsgChar, bufMsg.c_str());
    strcpy(modeMsgChar, modeMsg.c_str());

    sendMSG(mySocket, bufMsgChar);
    sendMSG(mySocket, modeMsgChar);

    char buffer[1];

    while(!exitFlag){
        if(serverMode == SERVER_MODE_NBYTE){
            char buffer2[packetSize];
            rez = "";
            if(readN(mySocket, buffer2, packetSize, rez) == 1)
                cout << arg->ip << ":" << arg->port << " " << rez << endl;
            else exitFlag = true;
        }else if(serverMode == SERVER_MODE_SEPARATOR){
            rez = "";
            if(readTillSeparator(mySocket, buffer, rez) == 1)
                cout << arg->ip << ":" << arg->port << " " << rez << endl;
            else exitFlag = true;
        }else exitFlag = true;
    }

    closeSocket(mySocket);

    return 0;
}

int serverProcess(){
    puts("\nServer process");

    while(true){
        string inputServer = "";
        cin >> inputServer;
        if(inputServer == "q"){
            cout << "Stoping" << endl;
            closesocket(listenSocket);

            /*
            int count=clientId.size();
            SOCKET socks[count];
            for(int i=0;i<count;i++)
                socks[i]=clientId[i];

            for(int i=0;i<count;i++){
                closeSocket(socks[i]);
            }
            */
            work = false;

            return 0;
        }else if(inputServer == "l"){
            cout << "client list:" << endl;
            for(int i = 0; i < clientId.size(); i++){
                cout << clientId[i] << "|" << clientIp[i] << ":" << clientPort[i] << endl;
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
        serverStart = true;
    }
    while(currentThreads < maxThreads){
        sockaddr_in clientInfo;
        int clientInfoSize = sizeof(clientInfo);
        SOCKET acceptSocket = accept(listenSocket, (struct sockaddr*)&clientInfo, &clientInfoSize);

        if(acceptSocket == INVALID_SOCKET){
                break;
            //closesocket(listenSocket);
            //WSACleanup();
            //ExitProcess(0);
        }

        char *ip = inet_ntoa(clientInfo.sin_addr);
        ArgsThread* args = new ArgsThread();
        args->port = clientInfo.sin_port;
        args->ip = string(ip);
        args->threadData = (void*)acceptSocket;

        printf("Connection request received.\nNew socket was created at address %s:%d\n", ip, clientInfo.sin_port);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)clientProcess, args, 0, NULL);
        currentThreads++;
    }
    if(currentThreads >= maxThreads){
            sockaddr_in clientInfo;
            int clientInfoSize = sizeof(clientInfo);
            SOCKET acceptSocket = accept(listenSocket, (struct sockaddr*)&clientInfo, &clientInfoSize);

            char fullMsg[] = "\nServer is full, connect later o/\n";
            sendMSG(acceptSocket, fullMsg);
            closesocket(acceptSocket);
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
                    if(arg > 0 && arg < 65) packetSize = arg;
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
                cout << "OPTIONS" << endl;
                cout << "-b, --buffer [1-64]        Buffer length, default: 8" << endl;
                cout << "-h, --help                 Show this message and close" << endl;
                cout << "-p, --port [1-65535]       Listen port, default: 8080" << endl;
                cout << "-s, --separator            Messages till separator" << endl;
                cout << "-t, --threads [1-10]       Maximum threads, default: 3" << endl << endl;
                cout << "SERVER OPTIONS" << endl;
                cout << "l                          List all online clients" << endl;
                cout << "k [number]                 Kill client" << endl;
                cout << "q                          Server shutdown" << endl;                
                return(0);
            }
        }
    }

    if(maxThreads == 0) maxThreads = MAX_THREADS_DEFAULT;
    if(serverPort == 0) serverPort = SERVER_PORT_DEFAULT;
    if(packetSize == 0) packetSize = PACKET_SIZE_DEAFULT;
    if(serverMode == 0) serverMode = SERVER_MODE_NBYTE;

    char temp[2];
    bufMsg = bufMsg + itoa(packetSize, temp, 10) + "\n";
    modeMsg = modeMsg + itoa(serverMode, temp, 10) + "\n";

    cout << "Server settings" << endl;
    cout << "Threads: " << maxThreads << endl;
    cout << "Port:    " << serverPort << endl;
    cout << "Buffer:  " << packetSize << endl;
    cout << "Mode:    " << serverMode << endl << endl;

    hMutex = CreateMutex(NULL, false, NULL);
    startWSA();

    struct sockaddr_in server;
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(serverPort);
    server.sin_family = AF_INET;

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

    //HANDLE threads[maxThreads];
    while(work)
        acceptConnections(listenSocket);

    //WSACleanup();
    return (EXIT_SUCCESS);
}