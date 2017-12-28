CXX   = g++
MINGW = i686-w64-mingw32-g++
CROSS = x86_64-unknown-linux-gnu-g++

FLAGS     = -I . -lpthread
WIN_FLAGS = -lwsock32 -static-libgcc -static-libstdc++
LIN_FLAGS = -std=c++11

SERVER_NAME = -o userver
CLIENT_NAME = -o uclient.exe

jenkins: jenkins-server jenkins-client

jenkins-server: CXX = ${CROSS}
jenkins-server: server.cpp server

jenkins-client: CXX = ${MINGW}
jenkins-client: client.cpp client

server: server.cpp
	${CXX} ${SERVER_NAME} $< ${FLAGS} ${LIN_FLAGS}

client: client.cpp
	${CXX} ${CLIENT_NAME} $< ${FLAGS} ${WIN_FLAGS}