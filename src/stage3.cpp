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

int main(){
    
    DIR* proc=opendir("/proc");
    
    if (proc == nullptr){
        cout << "Failed to open directory!" << endl;
        return 1;
    }

    dirent* entry ;
    
    cout << left << setw(8) << "PID" << "NAME" << endl;

    while ((entry = readdir(proc)) != nullptr){
        if (isPID(entry->d_name))
            cout << left << setw(8) << entry->d_name << getProcessName(entry->d_name) <<  endl;
    }

    closedir(proc);

    return 0;
}
