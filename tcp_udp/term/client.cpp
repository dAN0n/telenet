#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <unistd.h>

using namespace std;

char SERVER_IP[15] = "192.168.222.1";
int  SERVER_PORT   = 8080;
bool working       = true;
char buf[8];
int sock;

int recvServerMsg(string &line){
    int rc = 1;
    while(rc != 0){
        rc = recv(sock, buf, 1, 0);
        if(rc <= 0) return 0;
        if(buf[0] == '\n') return 1;
        line = line + buf[0];
    }
}

void clientProcess(string msg){   
    while(working){
        string msg;
        getline(cin, msg);

        if(msg == "quit"){
            working = false;
            close(sock);
            exit(0);            
        }

        msg += "\n";
        
        if(send(sock, msg.data(), msg.size(), 0) < 0){
            puts("Send error");
            working = false;
            close(sock);
            exit(1);
        }
    }
}

int main(int argc, char *argv[]){

    int opt, fullCheck;
    struct sockaddr_in server;
    string line;

    while( (opt = getopt(argc, argv, "hi:p:")) != -1){
        switch(opt){
            case 'h':
                cout << "OPTIONS"                          << endl;
                cout << "-i [ip]        Server IP address" << endl;
                cout << "-p [port]      Server port"       << endl;
                return(0);
            case 'i':
                snprintf(SERVER_IP, 15, "%s", optarg);
                break;
            case 'p':
                int port = atoi(optarg);
                if(port > 0 && port < 65536) SERVER_PORT = port;
        }
    }

    // cout << SERVER_IP   << endl;
    // cout << SERVER_PORT << endl;

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_port = htons(SERVER_PORT);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock < 0){
        perror("Socket error");
        return(1);
    }else puts("Socket created");

    if(connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0){
        perror("Connect error");
        return(2);
    }else puts("Connect success");

    thread t1(clientProcess, "thr");
    
    while(working){
        line = "";
        if(recvServerMsg(line) == 1){
            cout << line << endl;
            
            fullCheck = strncmp(line.data(), "\nServer is full", 15);
            if(fullCheck == 0) working = false;
        }
        else{
            cout << "You are disconnected!" << endl;
            working = false;
        }
    }

    // t1.join();
    t1.detach();
    close(sock);

    return 0;
}
