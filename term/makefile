CXX   = g++
MINGW = i686-w64-mingw32-g++
CROSS = x86_64-unknown-linux-gnu-g++

FLAGS     = -I . -lpthread
WIN_FLAGS = -lwsock32 -static-libgcc -static-libstdc++
LIN_FLAGS = -std=c++11

SERVER_NAME = -o server.exe
CLIENT_NAME = -o client

SERVER_CPP = server.cpp term.cpp

jenkins: jenkins-server jenkins-client

jenkins-server: CXX = ${MINGW}
jenkins-server: ${SERVER_CPP} server.h term.h server

jenkins-client: CXX = ${CROSS}
jenkins-client: client.cpp client

server: ${SERVER_CPP} server.h term.h
	${CXX} ${SERVER_NAME} ${SERVER_CPP} ${FLAGS} ${WIN_FLAGS}

client: client.cpp
	${CXX} ${CLIENT_NAME} $< ${FLAGS} ${LIN_FLAGS}