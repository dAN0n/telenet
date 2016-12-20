#include "server.h"

string MSG_WELCOME           = "Login or register new user (login/addusr LOGIN PASSWORD)\n";
string MSG_FULL              = "\nServer is full, connect later o/\n";
string MSG_LOGPASS_NOT_MATCH = "Login/password not match!\n";
string MSG_LOGPASS_MATCHING  = "Matching login/password are not allowed!\n";
string MSG_USER_EXISTS       = "This user already exists!\n";
string MSG_USER_NOT_EXISTS   = "This user doesn't exists!\n";
string MSG_USER_ONLINE       = "This user already online!\n";
string MSG_NO_PERMISSIONS    = "You doesn't have permissions for this command!\n";
string MSG_LOCK_PERMISSIONS  = "You can't change root and yours permissions!\n";
string MSG_LOCK_KILL         = "You can't kill root and yourself!\n";
string MSG_KILL_FAILED       = "This user is offline or not exists!\n";
string MSG_INVALID_CMD       = "Invalid command!\n";

struct clientDescriptor{
    SOCKET sock;
    HANDLE handle;
    char *ip;
    int port;
    string login;
};

struct userData{
    string login;
    string password;
    string permissions;
    string path;
    bool online;
};

int maxThreads;
int serverPort;
int packetSize;

clientDescriptor clientDesc[MAX_THREADS_POSSIBLE];
SOCKET listenSocket = INVALID_SOCKET;
const HANDLE hMutex = CreateMutex(NULL, false, NULL);

enum state{ CONNECT, WORK };
vector<userData> users;
string allPermissions = "ckw";

/***************
SERVER FUNCTIONS
***************/

void closeSocket(int ind){
    WaitForSingleObject(hMutex, INFINITE);
    if(ind >= 0 && ind < maxThreads && clientDesc[ind].sock != INVALID_SOCKET){
            if(closesocket(clientDesc[ind].sock))
                puts("Closesocket failed");
            else{
                cout << clientDesc[ind].ip << ":" << clientDesc[ind].port << " was disconnected" << endl;
                clientDesc[ind].sock = INVALID_SOCKET;
            }
    }
    ReleaseMutex(hMutex);
}

int recvS(SOCKET socket, char *buf, string &line){
    int rc = 1;
    while(rc != 0){
        rc = recv(socket, buf, 1, 0);
        if(rc <= 0) return 0;
        if(buf[0] == '\n') return 1;
        line = line + buf[0];
    }
}

int sendMSG(SOCKET socket, string buffer){
    if(send(socket, buffer.data(), buffer.size(), 0) <= 0) return 0;
    return 1;
}

DWORD WINAPI clientProcess(void* socket){
    int ind;
    bool exitFlag = false;
    state state = CONNECT;
    char buffer[packetSize];
    string line;

    for (int i = 0; i < maxThreads; i++){
        if (clientDesc[i].sock == (SOCKET)socket) ind = i;
    }

    while(!exitFlag){
        string login;
        string password;
        string cmd;
        int logInd;
        size_t pos = 0;
        line = "";

        switch(state){
            case CONNECT:

                sendMSG(clientDesc[ind].sock, MSG_WELCOME);

                if(recvS(clientDesc[ind].sock, buffer, line) != 1){
                    exitFlag = true;
                    break;
                }else cout << clientDesc[ind].ip << ":" << clientDesc[ind].port << " " << line << endl;

                pos = line.find(" ");
                cmd = line.substr(0, pos);

                if(cmd == "login" || cmd == "addusr"){
                    if(line.find_first_of(" ") != string::npos)
                        line = line.substr(line.find_first_of(" "), string::npos);
                    if(line.find_first_not_of(" ") != string::npos)
                        line = line.substr(line.find_first_not_of(" "), string::npos);
                    
                    pos = line.find(" ");
                    login = line.substr(0, pos);

                    if(line.find_first_of(" ") != string::npos)
                        line = line.substr(line.find_first_of(" "), string::npos);
                    if(line.find_first_not_of(" ") != string::npos)
                        line = line.substr(line.find_first_not_of(" "), string::npos);

                    pos = line.find(" ");
                    password = line.substr(0, pos);
                }
                else sendMSG(clientDesc[ind].sock, MSG_INVALID_CMD);
                
                if(cmd == "login"){
                    if(loginCommand(login, password) == 0){
                        logInd = getUserIndex(login);

                        WaitForSingleObject(hMutex,INFINITE);
                        clientDesc[ind].login = login;
                        ReleaseMutex(hMutex);

                        state = WORK;
                    }
                    else if(loginCommand(login, password) == 1){
                        sendMSG(clientDesc[ind].sock, MSG_LOGPASS_NOT_MATCH);
                        break;
                    }
                    else{
                        sendMSG(clientDesc[ind].sock, MSG_USER_ONLINE);
                        break;                        
                    }
                }

                if(cmd == "addusr"){
                    if(addusrCommand(login, password) == 0){
                        logInd = getUserIndex(login);

                        WaitForSingleObject(hMutex,INFINITE);
                        clientDesc[ind].login = login;
                        users[logInd].online = true;
                        ReleaseMutex(hMutex);

                        state = WORK;
                    }
                    else if(addusrCommand(login, password) == 1){
                        sendMSG(clientDesc[ind].sock, MSG_USER_EXISTS);
                        break;
                    }
                    else if(addusrCommand(login, password) == 2){
                        cout << "ERROR: Can't open users.txt" << endl;
                        return 2;
                    }
                    else{
                        sendMSG(clientDesc[ind].sock, MSG_LOGPASS_MATCHING);
                    }
                }

                break;
            case WORK:
                while(true){
                    string cmd;
                    vector<string> names;

                    string prompt = users[logInd].login + " @ " + users[logInd].path + " $ \n";
                    sendMSG(clientDesc[ind].sock, prompt);
                    
                    cmd = "";
                    if(recvS(clientDesc[ind].sock, buffer, cmd) != 1){
                        exitFlag = true;
                        break;
                    }else cout << clientDesc[ind].ip << ":" << clientDesc[ind].port
                               << "|" << clientDesc[ind].login << "| " << cmd << endl;

                    if(cmd == "ls"){
                        names = lsCommand(users[logInd].path);

                        for(int i = 0; i < names.size(); i++)
                            sendMSG(clientDesc[ind].sock, names[i] + "\n");
                        names.clear();
                    }

                    else if(cmd.find("cd ") != string::npos){
                        cmd = cmd.substr(3, string::npos);
                        cmd = cmd.substr(0, cmd.find_last_not_of(" ") + 1);

                        users[logInd].path = cdCommand(clientDesc[ind].sock, cmd, users[logInd].path);
                        rewriteUserFile();
                    }

                    else if(cmd == "pwd"){
                        sendMSG(clientDesc[ind].sock, users[logInd].path + "\n");
                    }

                    else if(cmd == "who"){
                        if(users[logInd].permissions.find("w") != string::npos){
                            for(int i = 0; i < users.size(); i++){
                                string online, send;
                                if(users[i].online) online = "ONLINE";
                                send = users[i].login + "\t" + online + "\t" + users[i].path + "\n";
                                sendMSG(clientDesc[ind].sock, send);
                            }
                        }else sendMSG(clientDesc[ind].sock, MSG_NO_PERMISSIONS);
                    }

                    else if(cmd.find("chmod ") != string::npos){
                        if(users[logInd].permissions.find("c") != string::npos){
                            cmd = cmd.substr(6, string::npos);
                            cmd = cmd.substr(0, cmd.find_last_not_of(" ") + 1);

                            pos = cmd.find(" ");
                            string login = cmd.substr(0, pos);

                            if(login == "root" || login == users[logInd].login){
                                sendMSG(clientDesc[ind].sock, MSG_LOCK_PERMISSIONS);
                                continue;
                            }

                            if(chmodCommand(cmd) == 0) rewriteUserFile();
                            else sendMSG(clientDesc[ind].sock, MSG_USER_NOT_EXISTS);
                        }else sendMSG(clientDesc[ind].sock, MSG_NO_PERMISSIONS);
                    }

                    else if(cmd.find("kill ") != string::npos){
                        if(users[logInd].permissions.find("k") != string::npos){
                            cmd = cmd.substr(5, string::npos);
                            cmd = cmd.substr(0, cmd.find_last_not_of(" ") + 1);

                            if(cmd == "root" || cmd == users[logInd].login){
                                sendMSG(clientDesc[ind].sock, MSG_LOCK_KILL);
                                continue;
                            }

                            if(killCommand(cmd) != 0) sendMSG(clientDesc[ind].sock, MSG_KILL_FAILED);
                        }else sendMSG(clientDesc[ind].sock, MSG_NO_PERMISSIONS);
                    }

                    else if(cmd == "logout"){
                        break;
                    }

                    else sendMSG(clientDesc[ind].sock, MSG_INVALID_CMD);
                }

                WaitForSingleObject(hMutex,INFINITE);
                users[logInd].online = false;
                clientDesc[ind].login = "";
                ReleaseMutex(hMutex);

                state = CONNECT;

                break;
            default:
                break;
        }
    }

    closeSocket(ind);
    return 0;
}

DWORD WINAPI serverProcess(){
    while(true){
        string inputServer;
        cin >> inputServer;

        if(inputServer == "q"){
            puts("Stopping server...");
            shutdown(listenSocket, SD_BOTH);
            closesocket(listenSocket);

            return 0;
        }else if(inputServer == "l"){
            puts("Client list:");

            WaitForSingleObject(hMutex, INFINITE);
            int lineCount = 0;
            for(int i = 0; i < maxThreads; i++){
                if(clientDesc[i].sock != INVALID_SOCKET){
                    cout << i << "|" << clientDesc[i].ip << ":" << clientDesc[i].port << "|" << clientDesc[i].login << endl;
                    lineCount++;
                }
            }
            if(lineCount == 0) puts("empty");
            ReleaseMutex(hMutex);
        }else if(inputServer == "k"){
            SOCKET sock;
            cin >> sock;
            if(cin.fail()) cin.clear();
            else closeSocket(sock);
        }
    }

    return 0;
}

DWORD WINAPI acceptConnections(void *listenSocket){
    SOCKET acceptSocket;
    clientDescriptor desc;
    sockaddr_in clientInfo;
    int clientInfoSize = sizeof(clientInfo);

    while(true){
        int ind = -1;

        acceptSocket = accept((SOCKET)listenSocket, (struct sockaddr*)&clientInfo, &clientInfoSize);

        if(acceptSocket == INVALID_SOCKET) break;
        else if(acceptSocket < 0){
            puts("Accept failed");
            continue;
        }

        WaitForSingleObject(hMutex, INFINITE);
        for (int i = maxThreads - 1; i >= 0; i--)
        if(clientDesc[i].sock == INVALID_SOCKET){
            ind = i;
        }
        ReleaseMutex(hMutex);

        if(ind < 0){
            char *ip  = inet_ntoa(clientInfo.sin_addr);

            sendMSG(acceptSocket, MSG_FULL);
            shutdown(acceptSocket, SD_BOTH);
            closesocket(acceptSocket);

            cout << ip << ":" << clientInfo.sin_port << " was disconnected because of server overload" << endl;
            continue;
        }

        desc.sock = acceptSocket;
        desc.ip   = inet_ntoa(clientInfo.sin_addr);
        desc.port = clientInfo.sin_port;

        cout << "Connection request received." << endl;
        cout << "New socket was created at address " << desc.ip << ":" << desc.port << endl;

        clientDesc[ind] = desc;
        clientDesc[ind].handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)clientProcess, (void *)desc.sock, 0, NULL);
    }

    WaitForSingleObject(hMutex, INFINITE);
    vector<HANDLE> hClients;
    for(int i = 0; i < maxThreads; i++)
        if(clientDesc[i].sock != INVALID_SOCKET){
            hClients.push_back(clientDesc[i].handle);
            closeSocket(i);
        }
    ReleaseMutex(hMutex);

    if (!hClients.empty()){
        WaitForMultipleObjects(hClients.size(), hClients.data(), TRUE, INFINITE);
    }
    return 0;
}

void startWSA(){
    WSADATA wsaDATA;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaDATA);
    if(iResult != 0){
        printf("WSAStartup failed with error: %d\n", iResult);
        exit(1);
    }
}

int main(int argc, char *argv[]){
    if(argc > 1){
        for(int i = 1; i < argc; i++){
            string opt = string(argv[i]);

            if(opt == "-p" || opt == "--port"){
                if(i < argc - 1){
                    int arg = atoi(argv[i + 1]);
                    if(arg > 0 && arg < 65536) serverPort = arg;
                }
            }
            else if(opt == "-b" || opt == "--buffer"){
                if(i < argc - 1){ 
                    int arg = atoi(argv[i + 1]);
                    if(arg > 0 && arg < 10) packetSize = arg;
                }
            }
            else if(opt == "-t" || opt == "--threads"){
                if(i < argc - 1){
                    int arg = atoi(argv[i + 1]);
                    if(arg > 0 && arg < 11) maxThreads = arg;
                }
            }
            else if(opt == "-h" || opt == "--help"){
                cout << endl;
                cout << "OPTIONS"                                                   << endl;
                cout << "-b, --buffer [1-9]         Buffer length, default: 8"      << endl;
                cout << "-h, --help                 Show this message and close"    << endl;
                cout << "-p, --port [1-65535]       Listen port, default: 8080"     << endl;
                cout << "-t, --threads [1-10]       Maximum threads, default: 2"    << endl << endl;
                cout << "SERVER OPTIONS"                                            << endl;
                cout << "l                          List all online clients"        << endl;
                cout << "k [number]                 Kill client"                    << endl;
                cout << "q                          Server shutdown"                << endl;
                return(0);
            }
        }
    }

    if(maxThreads == 0) maxThreads = MAX_THREADS_DEFAULT;
    if(serverPort == 0) serverPort = SERVER_PORT_DEFAULT;
    if(packetSize == 0) packetSize = PACKET_SIZE_DEFAULT;

    cout << "Server settings"         << endl;
    cout << "Threads: " << maxThreads << endl;
    cout << "Port:    " << serverPort << endl;
    cout << "Buffer:  " << packetSize << endl;
    cout << endl;

    if(readUserFile() == 1){
        cout << "ERROR: Can't open users.txt, creating new file with root:admin user" << endl;

        WaitForSingleObject(hMutex, INFINITE);
        ofstream ofs;
        ofs.open("users.txt");
        ofs << "root||admin||" << allPermissions << "||" << getServerPath() << endl;
        ofs.close();
        ReleaseMutex(hMutex);

        if(readUserFile() == 1){
            cout << "ERROR: Can't open users.txt" << endl;
        }
    }

    startWSA();

    WaitForSingleObject(hMutex, INFINITE);
    for (int i = 0; i < maxThreads; i++)
        clientDesc[i].sock = INVALID_SOCKET;
    ReleaseMutex(hMutex);

    sockaddr_in server;
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port        = htons(serverPort);
    server.sin_family      = AF_INET;

    if (listenSocket < 0){
        puts("Socket failed with error");
        WSACleanup();
        exit(2);
    }

    if(bind(listenSocket, (struct sockaddr *) &server, sizeof(server)) < 0){
        puts("Bind failed. Error");
        exit(3);
    }

    if(listen(listenSocket, SOCKET_ERROR) == SOCKET_ERROR){
        puts("Listen call failed. Error");
        exit(4);
    }

    HANDLE hAccept = CreateThread(NULL, 0, acceptConnections, (void *)listenSocket, 0, NULL);

    serverProcess();

    WaitForSingleObject(hAccept, INFINITE);

    WSACleanup();
    return 0;
}

/*****************
TERMINAL FUNCTIONS
*****************/

bool compareDir(string i, string j){
    bool iDir = false;
    bool jDir = false;

    if(i.find("/") != string::npos) iDir = true;
    if(j.find("/") != string::npos) jDir = true;

    if(iDir && !jDir) return 1;
    if(!iDir && jDir) return 0;
    else{
        if(i<j) return 1;
        else return 0;
    }
}

string getServerPath(){
    char buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, MAX_PATH);
    string::size_type pos = string(buffer).find_last_of("\\/");
    return string(buffer).substr(0, pos);
}

int rewriteUserFile(){
    WaitForSingleObject(hMutex, INFINITE);
    ofstream ofs;
    string filePath = getServerPath() + "/users.txt";

    ofs.open(filePath.data());

    if(ofs.is_open()){
        for(int i = 0; i < users.size(); i++){
            ofs << users[i].login << "||" << users[i].password << "||"
                << users[i].permissions << "||" << users[i].path << endl;
        }
    }else{
        ReleaseMutex(hMutex);

        return 1;
    }

    ofs.close();
    ReleaseMutex(hMutex);

    return 0;
}

vector<string> lsCommand(string folder){
    vector<string> names;
    string search_path = folder + "/*.*";
    WIN32_FIND_DATA fd; 
    HANDLE hFind = FindFirstFile(search_path.c_str(), &fd); 
    if(hFind != INVALID_HANDLE_VALUE){ 
        do{
            if(!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
                names.push_back(fd.cFileName);
            }
            else{
                string dir = fd.cFileName;
                if(!(dir == "." || dir == "..")){
                    dir += "/";
                    names.push_back(dir);
                }
            }
        }while(FindNextFile(hFind, &fd)); 
        FindClose(hFind);

        sort(names.begin(), names.end(), compareDir);
    } 
    return names;
}

string cdCommand(SOCKET sock, string dir, string path){
    vector<string> names;
    string subdir;

    size_t pos = dir.find_last_of("/");
    if(pos != dir.size() - 1) dir += "/";
    pos = 0;
    
    while((pos = dir.find("/")) != string::npos){
        subdir = dir.substr(0, pos);
        if(subdir == "..") path = path.substr(0, path.find_last_of("\\/"));
        else{
            names = lsCommand(path);
            if(find(names.begin(), names.end(), subdir + "/") != names.end())
                path += "/" + subdir;
            else{
                string msg = "Directory \"" + subdir + "\" is not exist\n";
                sendMSG(sock, msg);
                names.clear();
                break;
            }
            names.clear();
        }
        dir.erase(0, pos + 1);
    }

    return path;
}

int getUserIndex(string login){
    int ind = -1;

    WaitForSingleObject(hMutex, INFINITE);
    for(int i = 0; i < users.size(); i++){
        if(users[i].login == login){
            ind = i;
            break;
        }
    }
    ReleaseMutex(hMutex);

    return ind;
}

int readUserFile(){
    ifstream ifs;
    string line;
    string filePath = getServerPath() + "/users.txt";

    WaitForSingleObject(hMutex, INFINITE);
    ifs.open(filePath.data());

    if(ifs.is_open()){
        while(getline(ifs, line)){
            size_t pos = 0;
            string subline;
            vector<string> tempStrings;
            userData ud;

            while((pos = line.find("||")) != string::npos){
                subline = line.substr(0, pos);
                tempStrings.push_back(subline);
                line.erase(0, pos + 2);
            }
            tempStrings.push_back(line);

            ud.login       = tempStrings[0];
            ud.password    = tempStrings[1];
            ud.permissions = tempStrings[2];
            ud.path        = tempStrings[3];
            ud.online      = false;

            users.insert(users.end(), ud);
        }
    }else{
        ReleaseMutex(hMutex);

        return 1;
    }

    ifs.close();
    ReleaseMutex(hMutex);

    return 0;
}

int addusrCommand(string login, string password){
    WaitForSingleObject(hMutex, INFINITE);
    ofstream ofs;
    int ind = getUserIndex(login);
    string filePath = getServerPath() + "/users.txt";

    if(ind >= 0){
        return 1;
    }

    if(login == password) return 3;

    ofs.open(filePath.data(), ofstream::app);

    if(ofs.is_open()){
        ofs << login << "||" << password << "||||" << getServerPath() << endl;
    }else{
        ReleaseMutex(hMutex);

        return 2;
    }

    ofs.close();
    ReleaseMutex(hMutex);

    userData ud;

    ud.login       = login;
    ud.password    = password;
    ud.permissions = "";
    ud.path        = getServerPath();
    ud.online      = false;

    WaitForSingleObject(hMutex, INFINITE);
    users.insert(users.end(), ud);
    ReleaseMutex(hMutex);
    
    return 0;
}

int chmodCommand(string opt){
    string login;
    string permissions;
    size_t pos = 0;
    
    pos = opt.find(" ");
    login = opt.substr(0, pos);

    if(opt.find_first_of(" ") != string::npos)
        opt = opt.substr(opt.find_first_of(" "), string::npos);
    if(opt.find_first_not_of(" ") != string::npos)
        opt = opt.substr(opt.find_first_not_of(" "), string::npos);

    pos = opt.find(" ");
    opt = opt.substr(0, pos);

    for(int i = 0; i < opt.size(); i++){
        if(allPermissions.find(opt[i]) != string::npos)
            if(permissions.find(opt[i]) == string::npos)
                permissions += opt[i];
    }

    int ind = getUserIndex(login);

    if(ind >=0){
        WaitForSingleObject(hMutex, INFINITE);
        users[ind].permissions = permissions;
        ReleaseMutex(hMutex);
    }else return 1;

    return 0;
}

int killCommand(string login){
    int ind = -1;

    WaitForSingleObject(hMutex,INFINITE);
    for(int i = 0; i < maxThreads; i++){
        if(clientDesc[i].login == login){
            ind = i;
            break;
        }
    }

    if(ind >= 0){
        users[getUserIndex(login)].online = false;
        clientDesc[ind].login = "";
        ReleaseMutex(hMutex);

        closeSocket(ind);
    }else{
        ReleaseMutex(hMutex);
        return 1;
    }

    return 0;
}

int loginCommand(string login, string password){
    int ind = getUserIndex(login);

    if(ind >= 0){
        if(users[ind].online == true) return 2;
        if(users[ind].password == password){
            WaitForSingleObject(hMutex, INFINITE);
            users[ind].online = true;
            ReleaseMutex(hMutex);
        }else return 1;
    }else return 1;

    return 0;
}