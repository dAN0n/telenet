#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char MESSAGE[20] = "Message from server";
char buf[20];
int SERVER_PORT = 8080;

int main(void){
	WSADATA wsa;
	int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsa);
	printf("_WSAS start\n");
	if(wsaResult != 0){
		printf("WSAStartup error: %d\n", wsaResult);
		exit(1);
	}
	else
		puts("_WSAStartup success");

	struct sockaddr_in local;
	int s;
	int s1;
	int rc;
	
	local.sin_family = AF_INET;
	local.sin_port = htons(SERVER_PORT);
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	
	s = socket(AF_INET, SOCK_STREAM, 0);
	
	if(s < 0){
		perror("Socket error");
		exit(1);
	}else puts("_Socket created");
	
	rc = bind(s, (struct sockaddr *) &local, sizeof(local));
	
	if(rc < 0){
		perror("Bind error");
		exit(1);
	}else puts("_Bind success");
	
	rc = listen(s, 5);
	
	if(rc){
		perror("Listen error");
		exit(1);
	}else puts("_Listen");
	
	s1 = accept(s, NULL, NULL);
	
	if(s1 < 0){
		perror("Accept error");
		exit(1);
	}else puts("_Accept");
	
	rc = recv(s1, buf, sizeof(buf), 0);
	
	if(rc <= 0){
		perror("Recv error");
		exit(1);
	}else printf("%s\n", buf);
	
	rc = send(s1, MESSAGE, sizeof(MESSAGE), 0);
	
	if(rc <= 0)
		perror("Send error");
	else puts("_Send success");

	exit(0);
}
