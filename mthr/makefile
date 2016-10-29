server: server.cpp
	g++ -lpthread -o server server.cpp -I . -lwsock32

jenkins: server.cpp
	i686-w64-mingw32-g++ -o server.exe server.cpp -I . -lwsock32 -static-libgcc -static-libstdc++