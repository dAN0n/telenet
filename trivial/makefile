all: server client

server: serverw.c
	gcc -lpthread -o server serverw.c -I . -lwsock32

client: clientl.c
	gcc -lpthread -o client clientl.c -I .
