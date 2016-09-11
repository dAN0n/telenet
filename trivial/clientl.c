#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

char MESSAGE[20] = "Message from client";
char buf[20];
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
	}
	
	rc = connect(s, (struct sockaddr *) &peer, sizeof(peer));
	
	if(rc){
		perror("Connect error");
		exit(1);
	}
	
	rc = send(s, MESSAGE, sizeof(MESSAGE), 0);
	
	if(rc <= 0){
		perror("Send error");
		exit(1);
	}
	
	rc = recv(s, buf, sizeof(buf), 0);
	
	if(rc <= 0)
		perror("Recv error");
	else
		printf("%s\n", buf);
	
	exit(0);
}
