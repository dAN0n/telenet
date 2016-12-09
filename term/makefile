CXX   = g++
MINGW = i686-w64-mingw32-g++
CROSS = x86_64-unknown-linux-gnu-g++

FLAGS     = -I . -lpthread
WIN_FLAGS = -lwsock32 -static-libgcc -static-libstdc++
LIN_FLAGS = -std=c++11

SERVER_NAME = -o server.exe
CLIENT_NAME = -o client

jenkins: jenkins-server jenkins-client

jenkins-server: CXX = ${MINGW}
jenkins-server: server.cpp server.h server

jenkins-client: CXX = ${CROSS}
jenkins-client: client.cpp client

server: server.cpp server.h
	${CXX} ${SERVER_NAME} $< ${FLAGS} ${WIN_FLAGS}

client: client.cpp
	${CXX} ${CLIENT_NAME} $< ${FLAGS} ${LIN_FLAGS}