#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <iostream>

using namespace std;

char CONNECT_MESSAGE[17];
char SERVER_IP[15] = "192.168.222.1";
int SERVER_PORT = 8080;

int main(void){
	
	struct sockaddr_in peer;
	int s;
	int rc;
	
	peer.sin_family = AF_INET;
	peer.sin_addr.s_addr = inet_addr(SERVER_IP);
	peer.sin_port = htons(SERVER_PORT);
	
	s = socket(AF_INET, SOCK_STREAM, 0);
	
	if(s < 0){
		perror("Socket error");
		exit(1);
	}else puts("Socket created");
	
	rc = connect(s, (struct sockaddr *) &peer, sizeof(peer));
	
	if(rc){
		perror("Connect error");
		exit(1);
	}else puts("Connect success");

	rc = recv(s, CONNECT_MESSAGE, sizeof(CONNECT_MESSAGE), 0);
	cout << CONNECT_MESSAGE;
	rc = recv(s, CONNECT_MESSAGE, sizeof(CONNECT_MESSAGE), 0);
	cout << CONNECT_MESSAGE;

	while(1){
		string msg;
		cin >> msg;

		cout << msg << " " << msg.length() << endl;

		if(msg == "quit") break;

		char *sendPacket = new char[msg.length()];
		strcpy(sendPacket, msg.c_str());

		// rc = send(s, sendPacket, msg.length() + 1, 0);
		rc = send(s, sendPacket, msg.length(), 0);

		int remain = msg.length() - rc;
		cout << "rc: " << rc << endl;
		cout << "Send: " << remain << endl;
		char *tempPacket = sendPacket;

		while(remain > 0){
			tempPacket += rc;
			// rc = send(s, tempPacket, remain + 1, 0);
			rc = send(s, tempPacket, remain, 0);
			remain -= rc;
			cout << "rc: " << rc << endl;
			cout << "Send: " << remain << endl;
			}

		// if(rc <= 0){
			// perror("Send error");
			// exit(1);
		// }
	}

	exit(0);
}
