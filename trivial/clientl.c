#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

int main(void){
	
	struct sockaddr_in peer;
	int s;
	int rc;
	char buf[1];
	
	peer.sin_family = AF_INET;
	peer.sin_port = htons (8080);
	peer.sin_addr.s_addr = inet_addr("192.168.56.1");
	
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
	
	rc = send(s, "1", 1, 0);
	
	if(rc <= 0){
		perror("Send error");
		exit(1);
	}
	
	rc = recv(s, buf, 1, 0);
	
	if(rc <= 0)
		perror("Recv error");
	else
		printf("%c\n", buf[0]);
	
	exit(0);
}
