#include <cstdlib>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <ctime>
#include <sys/select.h> 

#define BUFLEN 1024
#define MAX_THREADS 2
#define SERVER_PORT_DEFAULT 8080
#define FAILED_ERROR "#FAILED"
#define MIXING_ERROR "#MIXING"
#define SAME_ERROR   "#SAME"

using namespace std;

int mainSocket;
pthread_mutex_t mutex;
pthread_t serverThread;

int serverPort;

bool serverStart = true;

struct timeval tmvl;

struct sockaddr_in serverAddress;

struct clientStruct{
    int id;
    char buffer[BUFLEN];
    sockaddr_in cl_sock;
};

pthread_t clients[MAX_THREADS];
struct clientStruct clientsData[MAX_THREADS];

int clientsConnected(){
    int count = 0;
    pthread_mutex_lock(&mutex);
    for(int i = 0; i < MAX_THREADS; i++){
        if(clientsData[i].id >= 0) count++;
    }
    pthread_mutex_unlock(&mutex);
    return count;
}

int findExistingClientsData(sockaddr_in &clientInfo){
    pthread_mutex_lock(&mutex);
    for(int i = 0; i < MAX_THREADS; i++){
        if(clientInfo.sin_port == clientsData[i].cl_sock.sin_port && clientsData[i].id >= 0){
            pthread_mutex_unlock(&mutex);
            return i;
        }
    }
    pthread_mutex_unlock(&mutex);
    return -1;
}

int findFreeClientsData(){
    pthread_mutex_lock(&mutex);
    for(int i = 0; i < MAX_THREADS; i++){
        if(clientsData[i].id < 0){
            pthread_mutex_unlock(&mutex);
            return i;
        }
    }
    pthread_mutex_unlock(&mutex);
    return -1;
}

void stopServer(){
    pthread_mutex_lock(&mutex);
    close(mainSocket);
    mainSocket = -1;
    serverStart = false;
    for(int i = 0; i < MAX_THREADS; i++)
        clientsData[i].id = -1;
    pthread_mutex_unlock(&mutex);
}

/* Send package */
void send(sockaddr_in clientInfo, string msg){
    int len = sizeof(clientInfo);
    sendto(mainSocket, msg.data(), msg.size(), 0, (struct sockaddr *) &clientInfo, len); // send package
}

/* Receive package (not using) */
string Receive(sockaddr_in &clientInfo){
    int len = sizeof(clientInfo);
    
    char buff[BUFLEN] = "";
    string res;
    
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(mainSocket, &rfds);

    int i = select(mainSocket + 1, &rfds, NULL, NULL, &tv);
    if(i > 0){
        recvfrom(mainSocket, buff, BUFLEN, 0, (struct sockaddr *) &clientInfo, (socklen_t *) &len);
        res = buff;
    }
    else{
        return FAILED_ERROR;
    }
    return res;
}

void* clientProcess(void *args){
    int id = *((int*) args);
    string temp = clientsData[id].buffer;
    
    int numSpace = temp.find(' ', 0);
    int idPackageInt = atoi(temp.substr(0, numSpace).data()) - 1;
    
    string buffer = "";
    clock_t t1, t2;
    bool needCheck = false;

    while(clientsData[id].id >= 0){
        if(buffer != clientsData[id].buffer){
            buffer = clientsData[id].buffer;
            string answer;

            if(count(buffer.begin(), buffer.end(), ' ') > 0){
                //поиск ID пакета
                int numSpace = buffer.find(' ', 0);
                string idPackage = buffer.substr(0, numSpace);
                buffer.erase(0, numSpace + 1);

                if(needCheck){
                    t2 = clock();
                    if(buffer == "#CHECK" && ((double)(t2 - t1) / CLOCKS_PER_SEC) < 3){
                        cout << "ID:" << id << " checked" << endl;
                    }
                    else
                        cout << "ID:" << id << " has not received a message" << endl;
                    needCheck = false;
                }
                else if(atoi(idPackage.data()) - idPackageInt == 1){
                    answer = "Echo: " + buffer + "\n";

                    idPackageInt = atoi(idPackage.data());
                    send(clientsData[id].cl_sock, idPackage + " " + answer);
                    needCheck = true;
                    t1 = clock();
                }
                else{
                    if(idPackageInt - atoi(idPackage.data()) == 0) answer = SAME_ERROR;
                    else answer = MIXING_ERROR;
                    send(clientsData[id].cl_sock, answer);
                    needCheck = true;
                    t1 = clock();
                }
            }
            else{
                answer = FAILED_ERROR;
                send(clientsData[id].cl_sock, answer);
                needCheck = true;
                t1 = clock();
            }
        buffer = clientsData[id].buffer;
        }
    }
    pthread_mutex_lock(&mutex);
    clientsData[id].id = -1;
    cout << "ID:" << id << " removed from list" << endl;
    pthread_mutex_unlock(&mutex);
}

void* serverProcess(void *){
    while(serverStart) {
        string inputServer;
        cin >> inputServer;

        if(!serverStart) break;
        else if(inputServer == "q"){
            puts("Stopping server...");
            stopServer();
            
            // exit(0);
        }
        else if(inputServer == "k"){
            int ind;
            cin >> ind;
            if(cin.fail()) cin.clear();

            pthread_mutex_lock(&mutex);
            if(ind < MAX_THREADS && ind >= 0 && clientsData[ind].id != -1) {
                clientsData[ind].id = -1;
            }
            else
                cout << "WRONG ID FOR KILL" << endl;
            pthread_mutex_unlock(&mutex);
        }
        else if(inputServer == "l"){
            cout << "Client list:" << endl;
            if(!clientsConnected()) puts("empty");
            else{
                for(int i = 0; i < MAX_THREADS; i++){
                    if(clientsData[i].id != -1){
                        pthread_mutex_lock(&mutex);
                        cout << clientsData[i].id << "|"
                             << inet_ntoa(clientsData[i].cl_sock.sin_addr)
                             << ":" << clientsData[i].cl_sock.sin_port << endl;
                        pthread_mutex_unlock(&mutex);
                    }
                }
            }
        }
    }
}


void processClients(){
    pthread_create(&serverThread, NULL, serverProcess, NULL);

    while(serverStart){
        struct sockaddr_in clientAddress;
        int size = sizeof(struct sockaddr_in);
        char buff[BUFLEN] = "";
        bool newThread = true;
        bool exist = false;

        recvfrom(mainSocket, buff, BUFLEN, 0, (sockaddr *) &clientAddress, (socklen_t *) &size);
        int pos = findExistingClientsData(clientAddress);
        if(pos >= 0) newThread = false;
        else pos = findFreeClientsData();

        if(pos >= 0){
            clientStruct client;
            client.id = pos;
            memcpy(client.buffer, buff, sizeof(client.buffer));
            client.cl_sock = clientAddress;

                pthread_mutex_lock(&mutex);
                clientsData[pos] = client;
                pthread_mutex_unlock(&mutex);

            if(newThread && serverStart){
                cout << "Add new client " << inet_ntoa(clientAddress.sin_addr)
                     << ":" << clientAddress.sin_port << endl;

                pthread_create(&clients[pos], NULL, clientProcess, (void*) &pos);
            }
        }else{
            cout << "SERVER OVERLOAD, KILL TO FREE THREAD" << endl;
        }
    }
}

int main(int argc, char *argv[]){

    int opt;

    while( (opt = getopt(argc, argv, "hp:")) != -1){
        switch(opt){
            case 'h':
                cout << endl;
                cout << "OPTIONS"                                                   << endl;
                cout << "-h, --help                 Show this message and close"    << endl;
                cout << "-p, --port [1-65535]       Listen port, default: 8080"     << endl;
                cout << "SERVER OPTIONS"                                            << endl;
                cout << "l                          List all online clients"        << endl;
                cout << "k [number]                 Kill client"                    << endl;
                cout << "q                          Server shutdown"                << endl;
                return(0);
            case 'p':
                int port = atoi(optarg);
                if(port > 0 && port < 65536) serverPort = port;
                break;
        }
    }

    if(serverPort == 0) serverPort = SERVER_PORT_DEFAULT;

    cout << "Server port: " << serverPort << endl << endl;

    for(int i = 0; i < MAX_THREADS; i++) clientsData[i].id = -1;

    pthread_mutex_init(&mutex, NULL);

    mainSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(mainSocket < 0){
        puts("Socket failed with error");
        return 1;
    }

    memset((char *) &serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(serverPort);

    if(bind(mainSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0){
        puts("Bind failed. Error");
        return 2;
    }

    processClients();

    int status_addr;

    pthread_join(serverThread, (void**) &status_addr);
    for(int i = 0; i < MAX_THREADS; i++) {
        pthread_join(clients[i], (void**) &status_addr);  // ЧТО-ТО СТРАННОЕ
    }

    close(mainSocket);
    mainSocket = -1;

    puts("Server is stopped.\n");
    return 0;
}
