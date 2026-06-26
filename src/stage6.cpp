#include <iostream>
#include <dirent.h>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>

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

long getMemory(char *pid,const string &field){
    string path="/proc/";
    path += pid;
    path += "/status";

    ifstream file(path);
    string line;

    while (getline(file,line)){
        if (line.find(field) == 0){
            stringstream ss(line);

            string label;
            long memory;

            ss >> label >> memory;
            return memory;
        }
    }

    return 0;
}

string formatMemory(long kb){
    stringstream ss;
    double ans;
    if (kb >= 1024*1024){
        ans = kb / (1024.0*1024.0);
        ss << fixed << setprecision(2) << ans << " GB";
        return ss.str();
    }
    ans = kb / (1024.0);
    ss << fixed << setprecision(2) << ans << " MB";
        return ss.str();
}

int main(){
    
    DIR* proc=opendir("/proc");
    
    if (proc == nullptr){
        cout << "Failed to open directory!" << endl;
        return 1;
    }

    dirent* entry ;
    
    cout << left << setw(8) << "PID" << setw(40) << "NAME" << setw(8) << "STATE" << setw(20) << "RES" << setw(20) << "VIRT" << endl;

    while ((entry = readdir(proc)) != nullptr){
        if (isPID(entry->d_name)){
            string ram = formatMemory(getMemory(entry->d_name,"VmRSS:"));
            string total = formatMemory(getMemory(entry->d_name,"VmSize:"));

            cout << left << setw(8) << entry->d_name << setw(40) << getProcessName(entry->d_name) << setw(8) << getProcessState(entry->d_name) << setw(20) << ram << setw(20) << total <<endl;
        }
    }
    closedir(proc);

    return 0;
}
