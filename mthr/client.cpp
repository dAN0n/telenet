#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <iostream>
#include <unistd.h>

using namespace std;

char CONNECT_MESSAGE[17];
char SERVER_IP[15] = "192.168.222.1";
int SERVER_PORT = 8080;

int main(int argc, char *argv[]){

	int opt;

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

	cout << SERVER_IP   << endl;
	cout << SERVER_PORT << endl;

	struct sockaddr_in peer;
	int s, rc, fullCheck, mode;

	peer.sin_family = AF_INET;
	peer.sin_addr.s_addr = inet_addr(SERVER_IP);
	peer.sin_port = htons(SERVER_PORT);

	s = socket(AF_INET, SOCK_STREAM, 0);

	if(s < 0){
		perror("Socket error");
		return(1);
	}else puts("Socket created");

	rc = connect(s, (struct sockaddr *) &peer, sizeof(peer));

	if(rc){
		perror("Connect error");
		return(2);
	}else puts("Connect success");

	rc = recv(s, CONNECT_MESSAGE, sizeof(CONNECT_MESSAGE), 0);
	cout << endl << CONNECT_MESSAGE;

	fullCheck = strncmp(CONNECT_MESSAGE, "\nServer is full", 15);

	rc = recv(s, CONNECT_MESSAGE, sizeof(CONNECT_MESSAGE), 0);
	cout << CONNECT_MESSAGE << endl;

	mode = atoi(CONNECT_MESSAGE + 15);
	if(fullCheck == 0) return(0);

	while(true){
		int cnt = 0;
		string msg;
		getline(cin, msg);
		// cout << msg << " " << msg.length() << endl;

		if(msg == "quit") break;

		while(cnt < msg.size()){
			if(mode == 0)
				rc = send(s, msg.data() + cnt, msg.size() + cnt, 0);
			else rc = send(s, msg.data() + cnt, msg.size() + cnt + 1, 0);

			cnt += rc;

			// cout << "rc: " << rc << endl;
			// cout << "cnt: " << cnt << endl;
			}

		if(rc <= 0){
			perror("Send error");
			return(3);
		}
	}

	return(0);
}
