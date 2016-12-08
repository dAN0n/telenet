#pragma once
#include <windows.h>
#include <stdlib.h>
#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <clocale>
#include <conio.h>
using namespace std;
 
int terminal();
int rewriteUserFile();
int getUserIndex(string login);
int readUserFile();
int addusrCommand(string login, string password);
int rmusrCommand(string login);
int loginCommand(string login, string password);
int chmodCommand(string opt);
string getServerPath();
string cdCommand(string dir, string path);
vector<string> lsCommand(string folder);
bool compareDir(string i, string j);