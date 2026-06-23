#include <iostream>
#include <dirent.h>
#include <cctype>
#include <fstream>
#include <iomanip>

using namespace std;

bool isPID(char *name){
    int i=0;
    while(name[i] != '\0'){
        if(isdigit(name[i]) == 0)
            return false;
        i++;
    }
    return true;
}

string getProcessName(char *pid){
    string path="/proc/";
    path += pid;
    path += "/comm";

    ifstream f(path);
    string name;
    getline(f,name);

    return name;
}

char getProcessState(char *pid){
    string path="/proc/";
    path += pid;
    path += "/status";

    ifstream file(path);

    string line;

    while (getline(file,line)){
        if (line.find("State:") == 0)
            return line[7]; //Status: S (sleeping)
    }

    return '?';
}

int main(){
    
    DIR* proc=opendir("/proc");
    
    if (proc == nullptr){
        cout << "Failed to open directory!" << endl;
        return 1;
    }

    dirent* entry ;
    
    cout << left << setw(8) << "PID" << setw(35) << "NAME" << "STATE" << endl;

    while ((entry = readdir(proc)) != nullptr){
        if (isPID(entry->d_name))
            cout << left << setw(8) << entry->d_name << setw(30) << getProcessName(entry->d_name) << getProcessState(entry->d_name) <<endl;
    }

    closedir(proc);

    return 0;
}
