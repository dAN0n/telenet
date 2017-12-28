#include <winsock2.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <algorithm>

#pragma comment(lib, "wsock32.lib")

#define BUFFER_SIZE  1024

using namespace std;

char SERVER_IP[15] = "192.168.222.15";
int SERVER_PORT = 8080;
bool closeClient;
char buf[BUFFER_SIZE];

sockaddr_in server;
SOCKET clientSocket;
HANDLE hMutex; // Дескриптор mutex
HANDLE threads;

int startWSA(){
    WSADATA wsaDATA;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaDATA);
    if(iResult != 0){
        printf("WSAStartup failed with error: %d\n", iResult);
        exit(1);
    }
}

// void verifyConnection(){ /*не фурычит*/
//     while(!closeClient){
//         Sleep(2000);
//  
//         int len = sizeof(server);
//         string checker="#VERIFY";
//         sendto(clientSocket, checker.data(), checker.size(), 0, (struct sockaddr *) &server, len);
//     }
// }

/* Receive package */
string receiveData(sockaddr_in &server) {
    memset(buf, 0, BUFFER_SIZE);

    int len = sizeof(server);

    struct timeval tv;
    tv.tv_sec = 2;                                              // time to wait 2 sec
    tv.tv_usec = 0;

    fd_set rfds;                                                // проверяем наличие данных в сокете, установив файловый дескриптор сокета в множестве "на чтение"
    FD_ZERO(&rfds);                                             // очищаем множество "на чтение"
    FD_SET(clientSocket, &rfds);                                // помещаем дескриптор сокета в множество "на чтение"

    int i = select(clientSocket + 1, &rfds, NULL, NULL, &tv);   // контроллируем активность сокета

    if(i > 0){                                                // если таймаут не успел завершиться
        recvfrom(clientSocket, buf, BUFFER_SIZE, 0, (struct sockaddr *) &server, &len);
        string res(buf);
        return res;
    }else{
        cout << "TIMEOUT" << endl;
        return "";
    }
    return "";
}

/* Send package */
int sendData(sockaddr_in server, string msg, int id_package) {
    memset(buf, 0, BUFFER_SIZE);

    string res = "";

    char buffer[8];
    sprintf(buffer, "%d", id_package);
    string id = buffer;

    res = id + " " + msg;                                  // package

    int len = sizeof(server);
    sendto(clientSocket, res.data(), res.size(), 0, (struct sockaddr *) &server, len); // send package
    string getAnswer = receiveData(server);

    if(getAnswer.empty()){
        cout << "PACKAGE " + id + " WAS LOST" << endl; // write about lost message
        cout << "NO CONNECTION" << endl;
    }
    else{
        int numSpace = getAnswer.find(" ", 0);
        if(numSpace < 0){
            cout << "PACKAGE " + id + " WAS LOST" << endl; // write about lost message
            cout << "RECEIVED: " + getAnswer << endl;
        }
        else{
            int id_getPackage = atoi(getAnswer.substr(0, numSpace).data());
            if(id_getPackage != id_package){
                cout << "PACKAGE " + id + " WAS LOST" << endl; // write about lost message
                cout << "RECEIVED: " + getAnswer << endl;
            }
            else{
                getAnswer.erase(0, numSpace + 1);
                cout << getAnswer << endl;
            }
        }
        res = id + " #CHECK";
        sendto(clientSocket, res.data(), strlen(res.data()), 0, (struct sockaddr *) &server, len); // send package
    }
    return 0;
}

int main(int argc, char *argv[]){
    if(argc > 1){
        for(int i = 1; i < argc; i++){
            string opt = string(argv[i]);

            if(opt == "-h" || opt == "--help"){
                cout << "OPTIONS"                          << endl;
                cout << "-i [ip]        Server IP address" << endl;
                cout << "-p [port]      Server port"       << endl;
                return(0);
            }
            else if(opt == "-p"){
                if(i < argc - 1){
                    int arg = atoi(argv[i + 1]);
                    if(arg > 0 && arg < 65536) SERVER_PORT = arg;
                }
            }
            else if(opt == "-i"){
                if(i < argc - 1){
                    snprintf(SERVER_IP, 15, "%s", argv[i + 1]);
                }
            }
        }
    }

    hMutex = CreateMutex(NULL, false, NULL); // атрибут безопасности, флаг начального владельца, имя объекта
 
    startWSA();

    clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //домен AF_INET – для передачи с использованием стека протоколов TCP/IP

    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_port        = htons(SERVER_PORT);
    server.sin_family      = AF_INET;

    if(clientSocket == INVALID_SOCKET){
        puts("Socket failed with error");
        WSACleanup();
        return 2;
    }

    string cmd;
    int id_package = 0;
    cout << "Use phrase \"quit\" for exit" << endl;

    // CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) verifyConnection, NULL, 0, NULL); /*не фурычит*/

    while(!closeClient){
        getline(cin, cmd);
        if(cmd == "quit") closeClient = true;
        else{
            sendData(server, cmd, ++id_package);
            // if(id_package == 2) id_package++;
            // if(id_package == 4) id_package = 2;
            // if(id_package == 2) id_package--;
        }
    }

    if(clientSocket != INVALID_SOCKET){
        if(closesocket(clientSocket)){
            puts("Socket closing failed with error");
        }
    }

    WSACleanup();
    return 0;
}