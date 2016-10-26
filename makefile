server: main.cpp transfer.cpp
	g++ -lpthread -o server main.cpp transfer.h transfer.cpp -I . -lwsock32