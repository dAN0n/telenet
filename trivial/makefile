all: client jenkins test

server: serverw.c
	gcc -lpthread -o server serverw.c -I . -lwsock32

client: clientl.c
	gcc -lpthread -o client clientl.c -I .

jenkins: serverw.c
	i686-w64-mingw32-gcc -o server.exe serverw.c -I . -lwsock32