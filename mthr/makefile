all: client jenkins

server: server.cpp
	g++ -lpthread -o server server.cpp -I . -lwsock32

client: client.cpp
	g++ -lpthread -o client client.cpp -I .

jenkins: server.cpp
	i686-w64-mingw32-g++ -o server.exe server.cpp -I . -lwsock32 -static-libgcc -static-libstdc++
