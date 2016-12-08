#include "term.h"

struct userData{
    string login;
    string password;
    string permissions;
    string path;
    bool online;
};

enum state{ CONNECT, WORK };
vector<userData> users;
string allPermissions;
// string allPermissions = "ckrw";

bool compareDir(string i, string j){
    bool iDir, jDir = false;

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
    ofstream ofs;
    string filePath = getServerPath() + "/users.txt";

    ofs.open(filePath.data());

    if(ofs.is_open()){
        for(int i = 0; i < users.size(); i++){
            ofs << users[i].login << "||" << users[i].password << "||"
                << users[i].permissions << "||" << users[i].path << endl;
        }
        return 0;
    }else return 1;
    ofs.close();
}

vector<string> lsCommand(string folder){
    vector<string> names;
    string search_path = folder + "/*.*";
    WIN32_FIND_DATA fd; 
    HANDLE hFind = FindFirstFile(search_path.c_str(), &fd); 
    if(hFind != INVALID_HANDLE_VALUE){ 
        do { 
            // read all (real) files in current folder
            // , delete '!' read other 2 default folder . and ..
            if(! (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
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

string cdCommand(string dir, string path){
    vector<string> names;
    string subdir;
    
    // dir = dir.substr(2, string::npos);
    // if(dir.find_first_not_of(" ") != string::npos)
    //     dir = dir.substr(dir.find_first_not_of(" "), string::npos);
    // dir = dir.substr(0, dir.find_last_not_of(" ") + 1);

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
                cout << "Directory \"" << subdir << "\" is not exist" << endl;
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

    for(int i = 0; i < users.size(); i++){
        if(users[i].login == login){
            ind = i;
            break;
        }
    }

    return ind;
}

int readUserFile(){
    ifstream ifs;
    string line;
    string filePath = getServerPath() + "/users.txt";

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

        ifs.close();

        return 0;

        // cout << "File open" << endl;
        // for(int i = 0; i < users.size(); i++){
            // cout << users[i].login << " " << users[i].password << " " << users[i].permissions << " " << users[i].path << " " << users[i].online << endl;
        // }
    }else{
        // cout << "Error. Can not open users.txt" << endl;
        return 1;
    }
}

int addusrCommand(string login, string password){
    ofstream ofs;
    int ind = getUserIndex(login);
    string filePath = getServerPath() + "/users.txt";

    if(ind >= 0){
        // cout << "This user already exists!" << endl << endl;
        return 1;
    }

    ofs.open(filePath.data(), ofstream::app);

    if(ofs.is_open()){
        ofs << login << "||" << password << "||||" << getServerPath() << endl;
    //     cout << "User " << login << " was created" << endl;
    }else return 2;
    ofs.close();

    userData ud;

    ud.login       = login;
    ud.password    = password;
    ud.permissions = "";
    ud.path        = getServerPath();
    ud.online      = false;

    users.insert(users.end(), ud);
    
    return 0;
}

int rmusrCommand(string login){
    int ind = getUserIndex(login);

    if(ind >= 0){
        users.erase(users.begin() + ind);
        return 0;
    }else return 1;
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

    if(login == "root") return 1;

    for(int i = 0; i < opt.size(); i++){
        if(allPermissions.find(opt[i]) != string::npos)
            if(permissions.find(opt[i]) == string::npos)
                permissions += opt[i];
    }

    int ind = getUserIndex(login);

    users[ind].permissions = permissions;

    return 0;
}

int loginCommand(string login, string password){
    int ind = getUserIndex(login);

    if(ind >= 0){
        if(users[ind].password == password){
            users[ind].online = true;
        }else return 1;
    }else return 1;

    return 0;
}


int terminal(){
    setlocale(LC_ALL, "Russian");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    bool exitFlag = false;
    state state = CONNECT;
    string line;

    if(readUserFile() == 1){
        cout << "ERROR: Can't open users.txt, creating new file with root:root user" << endl;

        ofstream ofs;
        ofs.open("users.txt");
        ofs << "root||root||" << allPermissions << "||" << getServerPath() << endl;
        ofs.close();

        if(readUserFile() == 1){
            cout << "ERROR: Can't open users.txt" << endl;
        }

        // return 1;
    }

    while(!exitFlag){
        string login;
        string password;
        string cmd;
        int ind;
        size_t pos = 0;

        switch(state){
            case CONNECT:

                getline(cin, line);

                if(line == "quit"){
                    return 0;
                }

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
                
                if(cmd == "login"){
                    if(loginCommand(login, password) == 0){
                        ind = getUserIndex(login);
                        state = WORK;
                    }
                    else{
                        cout << "User/password not match!" << endl;
                        break;
                    }
                }

                if(cmd == "addusr"){
                    if(addusrCommand(login, password) == 0){
                        ind = getUserIndex(login);
                        users[ind].online = true;
                        state = WORK;
                    }
                    else if(addusrCommand(login, password) == 1){
                        cout << "This user already exists!" << endl;
                        break;
                    }
                    else if(addusrCommand(login, password) == 2){
                        cout << "ERROR: Can't open users.txt, creating" << endl;
                        return 2;
                        // break;
                    }
                }

                break;
            case WORK:
                while(true){
                    string cmd;
                    vector<string> names;

                    cout << users[ind].login << " @ " << users[ind].path << " $ ";
                    getline(cin, cmd);

                    if(cmd == "ls"){
                        names = lsCommand(users[ind].path);
                        for(int i = 0; i < names.size(); i++)
                            cout << names[i] << endl;
                        names.clear();
                    }

                    else if(cmd.find("cd ") != string::npos){
                        cmd = cmd.substr(2, string::npos);
                        if(cmd.find_first_not_of(" ") != string::npos)
                            cmd = cmd.substr(cmd.find_first_not_of(" "), string::npos);
                        cmd = cmd.substr(0, cmd.find_last_not_of(" ") + 1);

                        users[ind].path = cdCommand(cmd, users[ind].path);
                        rewriteUserFile();
                    }

                    else if(cmd == "pwd") cout << users[ind].path << endl;

                    else if(cmd == "who"){
                        if(users[ind].permissions.find("w") != string::npos){
                            for(int i = 0; i < users.size(); i++){
                                string online;
                                if(users[i].online) online = "ONLINE";
                                cout << users[i].login << "\t" << online << "\t" << users[i].path << endl;
                            }
                        }else cout << "You doesn't have permissions for this command!" << endl;
                    }

                    else if(cmd.find("chmod ") != string::npos){
                        if(users[ind].permissions.find("c") != string::npos){
                            cmd = cmd.substr(5, string::npos);
                            if(cmd.find_first_not_of(" ") != string::npos)
                                cmd = cmd.substr(cmd.find_first_not_of(" "), string::npos);
                            cmd = cmd.substr(0, cmd.find_last_not_of(" ") + 1);

                            if(chmodCommand(cmd) == 0) rewriteUserFile();
                            else cout << "You can't change root permissions!" << endl;
                        }else cout << "You doesn't have permissions for this command!" << endl;
                    }

                    // // TODO: Учесть, что юзер может быть залогинен (предварительно дискон)
                    // else if(cmd.find("rmusr ") != string::npos){
                    //     if(users[ind].permissions.find("r") != string::npos){
                    //         cmd = cmd.substr(5, string::npos);
                    //         if(cmd.find_first_not_of(" ") != string::npos)
                    //             cmd = cmd.substr(cmd.find_first_not_of(" "), string::npos);
                    //         cmd = cmd.substr(0, cmd.find_last_not_of(" ") + 1);

                    //         if(cmd == users[ind].login)
                    //             cout << "You can't remove yourself!" << endl;
                    //         else if(cmd == "root")
                    //             cout << "You can't remove root user!" << endl;
                    //         // как-то выйти

                    //         if(rmusrCommand(cmd) == 0) rewriteUserFile();
                    //         else cout << "This user doesn't exists!" << endl;
                    //     }else cout << "You doesn't have permissions for this command!" << endl;
                    // }

                    else if(cmd == "quit" || cmd == "exit"){
                        users[ind].online = false;
                        return 0;
                    }

                    else if(cmd == "logout"){
                        users[ind].online = false;
                        break;
                    }
                }

                state = CONNECT;

                break;
            default:
                break;
        }
    }

        // Логин
        // cout << "Login: ";
        // getline(cin, login);

        // cout << "Password: ";
        // while((pw = getch()) != '\r'){
        //     password += pw;
        //     // cout << pw << endl;
        // }
        // cout << endl;

}