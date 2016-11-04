CXX = g++
MINGW = i686-w64-mingw32-g++
CROSS = x86_64-unknown-linux-gnu-g++

FLAGS     = -lpthread -I .
WIN_FLAGS = -lwsock32 -static-libgcc -static-libstdc++

SERVER_NAME = -o server.exe
CLIENT_NAME = -o client

jenkins: jenkins-server jenkins-client

jenkins-server: CXX = ${MINGW}
jenkins-server: server.cpp server

jenkins-client: CXX = ${CROSS}
jenkins-client: client.cpp client

server: server.cpp
	${CXX} ${FLAGS} ${WIN_FLAGS} ${SERVER_NAME} $<

client: client.cpp
	${CXX} ${FLAGS} ${CLIENT_NAME} $<
