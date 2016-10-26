#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cstring>
#include <iostream>
#include "transfer.cpp"

#define MAX_THREADS_DEFAULT 3
#define SERVER_PORT_DEFAULT 8080
#define PACKET_SIZE_DEAFULT 8

#pragma comment(lib,"Ws2_32.lib")

using namespace std;

HANDLE hMutex;
int current_threads=0;
bool readn;
bool work=true;
bool serverStart=false;
char welcomeMsg[]="Welcome to the server!\nHow you want to work?\n1 for n byte's messages\n2 for messages till separator\n";
char nMsg[]="Type n for n symbols session\n";
char helpMsg[]="Commands:\nshow - show all online clients\nstop - stop server\nkill - kill client\n";
int n_length;

vector<int> clientId;
vector<string> clientIp;
vector<u_short> clientPort;
HANDLE serverThread;

SOCKET listenSocket=INVALID_SOCKET;

struct ArgsThread{
    void* threadData;
    string ip;
    u_short port;
};

void startWSA(){
    WSADATA wsaDATA;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaDATA);
    if(iResult != 0){
        printf("WSAStartup failed with error: %d\n", iResult);
        exit(1);
    }else
        puts("WSAStartup success");
}

void closeSocket(SOCKET sock){
    WaitForSingleObject(hMutex,INFINITE);
    int kill_index=-1;
    for(int i=0;i<clientId.size();i++)
        if(clientId[i]==sock)
            kill_index=i;
    if(kill_index>=0){
        string ip=clientIp[kill_index];
        u_short port=clientPort[kill_index];
        clientId.erase(clientId.begin()+kill_index);
        clientIp.erase(clientIp.begin()+kill_index);
        clientPort.erase(clientPort.begin()+kill_index);

        int temp=closesocket(sock);
        if(temp){
            cout<<"Error. Accept socket was not closed"<<endl;
        }else{
            cout<<ip<<":"<<port<<" disconnected"<<endl;
            current_threads--;
        }
    }
    ReleaseMutex(hMutex);
}
int clientProcess(ArgsThread* arg){
    SOCKET mySocket=(SOCKET)arg->threadData;
    WaitForSingleObject(hMutex,INFINITE);
    clientId.insert(clientId.end(),mySocket);
    clientIp.insert(clientIp.end(),arg->ip);
    clientPort.insert(clientPort.end(),arg->port);
    ReleaseMutex(hMutex);
    bool exitFlag=false;
    string rez="";
    transfer tr;
    tr.socket=&mySocket;
    tr.sendMSG(welcomeMsg);
    char buffer[1];
    if(tr.readTillSeparator(buffer,rez)!=1){
        closeSocket(mySocket);
        exitFlag=true;
    }
    int check=atoi(rez.c_str());
    bool nSetup=false;
    while(!exitFlag){
        if(mySocket==INVALID_SOCKET)
            cout<<"hgg"<<endl;
        if(check==1){
            if(!nSetup){
                tr.sendMSG(nMsg);
                rez="";
                if(tr.readTillSeparator(buffer,rez)==1)
                    n_length=atoi(rez.c_str());
                else{
                    closeSocket(mySocket);
                    exitFlag=true;
                }
                if(n_length==0){
                    closeSocket(mySocket);
                    exitFlag=true;
                }
                nSetup=true;
            }else{
                char buffer2[n_length];
                rez="";
                if(tr.readN(buffer2,n_length,rez)==1)
                    cout<<arg->ip<<":"<<arg->port<<" "<<rez<<endl;
                else{
                    closeSocket(mySocket);
                    exitFlag=true;
                }
            }
        }else if(check==2){
            rez="";
            if(tr.readTillSeparator(buffer,rez)==1)
                cout<<arg->ip<<":"<<arg->port<<" "<<rez<<endl;
            else{
                closeSocket(mySocket);
                exitFlag=true;
            }
        }else{
            closeSocket(mySocket);
            exitFlag=true;
        }
    }
    return 0;
}
int serverProcess(){
    puts("Server process");
    string commandHelp="help";
    string commandKill="kill";
    string commandShow="show";
    string commandStop="stop";

    while(true){
        string inputServer="";
        cin>>inputServer;
        if(strcmp(inputServer.c_str(),commandStop.c_str())==0){
            cout<<"Stoping"<<endl;
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
            work=false;

            return 0;
        }else if(strcmp(inputServer.c_str(),commandHelp.c_str())==0){
            cout<<helpMsg<<endl;
        }else if(strcmp(inputServer.c_str(),commandShow.c_str())==0){
            cout<<"client list:"<<endl;
            for(int i=0;i<clientId.size();i++){
                cout<<clientId[i]<<"|"<<clientIp[i]<<":"<<clientPort[i]<<endl;
            }
        }else if(strcmp(inputServer.c_str(),commandKill.c_str())==0){
            SOCKET sock;
            cin>>sock;
            closeSocket(sock);
        }
    }

    return 0;
}
void acceptConnections(SOCKET listenSocket){
    if(!serverStart){
        serverThread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)serverProcess,NULL,0,NULL);
        serverStart=true;
    }
    while(current_threads<MAX_THREADS_DEFAULT){
        sockaddr_in clientInfo;
        int clientInfoSize=sizeof(clientInfo);
        SOCKET acceptSocket=accept(listenSocket,(struct sockaddr*)&clientInfo,&clientInfoSize);
        if(acceptSocket==INVALID_SOCKET){
                break;
            //closesocket(listenSocket);
            //WSACleanup();
            //ExitProcess(0);
        }
        char *ip=inet_ntoa(clientInfo.sin_addr);
        ArgsThread* args=new ArgsThread();
        args->port=clientInfo.sin_port;
        args->ip=string(ip);
        args->threadData=(void*)acceptSocket;


        printf("Connection request received.\nNew socket was created at address %s:%d\n",ip,clientInfo.sin_port);
        CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)clientProcess,args,0,NULL);
        current_threads++;

    }
}
int main(){
    hMutex=CreateMutex(NULL,false,NULL);
    startWSA();

    struct sockaddr_in server;
    listenSocket=socket(AF_INET, SOCK_STREAM,0);

    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(SERVER_PORT_DEFAULT);
    server.sin_family = AF_INET;

    if (listenSocket <0){
        puts("Socket failed with error");
        WSACleanup();
        exit(1);
    }else
        puts("Socket created");

    if(bind(listenSocket, (struct sockaddr * )&server, sizeof(server))<0){
        puts("Bind failed. Error");
        exit(1);
    }else
        puts("Bind created");

    if(listen(listenSocket, SOCKET_ERROR)==SOCKET_ERROR){
        puts("Listen call failed. Error");
        exit(1);
    }else
        puts("Listen started");

    //HANDLE threads[MAX_THREADS_DEFAULT];
    while(work)
        acceptConnections(listenSocket);


    //WSACleanup();
    return (EXIT_SUCCESS);
}
