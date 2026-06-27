#include <iostream>
#include <dirent.h>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#include <algorithm>

using namespace std;

struct ProcessInfo{
    string pid;
    string name;
    char state;
    string res;
    string virt;
    double cpu;
};

bool isPID(char *name){
    int i=0;
    while(name[i] != '\0'){
        if(isdigit(name[i]) == 0)
            return false;
        i++;
    }
    return true;
}

string getProcessName(const string &pid){
    string path="/proc/";
    path += pid;
    path += "/comm";

    ifstream f(path);
    string name;
    getline(f,name);

    return name;
}

char getProcessState(const string &pid){
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

long getCPUTime(const string &pid){
    //1.PID 2.Name 3.State 4.ParentPID .... 14.Utime 15.Stime


    string path = "/proc/" + pid +"/stat";

    ifstream file(path);

    if(!file)
        return 0;

    string line;
    getline(file, line);

    if(line.empty())
        return 0;

    size_t pos = line.find(')');
    line = line.substr(pos+2); //starting from state;

    stringstream ss(line);

    string state;
    ss >> state; //3

    long temp; //4
    for (int i=0;i<10;i++){
        ss >> temp;
    }
    // 13
    long utime,stime;
    
    ss >> utime;
    ss >> stime;

    return utime+stime;
}

long getTotalCPUTime(){
    ifstream file("/proc/stat");
    string line;
    getline(file,line);

    stringstream ss(line);

    string cpu;
    long time;
    long total=0;

    ss >> cpu;
    while (ss >> time)
        total += time;

    return total;
}

void cpuCalc(const string &pid){
    long pro1=getCPUTime(pid);
    long total1=getTotalCPUTime();
    
    sleep(1);

    long pro2=getCPUTime(pid);
    long total2=getTotalCPUTime();

    long delta_pro = pro2 - pro1;
    long delta_total = total2 - total1;

    double cpuPercent = ((double)delta_pro/delta_total)*100.0;

    cout << fixed << setprecision(2) << cpuPercent << "%" << endl;

}

unordered_map <string,long> getSnapshot(){
    unordered_map <string,long> snapShot;

    DIR* proc=opendir("/proc");

    if (proc == nullptr)
        return snapShot;
    
    dirent* entry ;

    while ((entry = readdir(proc)) != nullptr){
        
        if (isPID(entry->d_name)){
            
            string pid = entry->d_name;
            snapShot[pid] = getCPUTime(pid);
        
        }
    }

    closedir(proc);

    return snapShot;
}

double getCPUpercent(const string &pid, const unordered_map<string,long> &oldT, const unordered_map<string,long> &newT, long total1, long total2){

    long deltaTick,deltaTotal;
    double ans;

    if (oldT.find(pid) == oldT.end() || newT.find(pid) == newT.end())
        return 0.00;

    deltaTick = newT.at(pid) - oldT.at(pid);
    deltaTotal = total2 - total1;

    if (deltaTotal <= 0)
        return 0.00;

    ans = (double)deltaTick*100.0/deltaTotal;

    return ans;
}

int main(){
    
    DIR* proc=opendir("/proc");
    
    if (proc == nullptr){
        cout << "Failed to open directory!" << endl;
        return 1;
    }

    dirent* entry ;

    auto oldT = getSnapshot();
    long total1 = getTotalCPUTime();

    sleep(1);

    auto newT = getSnapshot();
    long total2 = getTotalCPUTime();
    
    ProcessInfo p;
    vector <ProcessInfo> processes;
    

    cout << left
     << setw(8) << "PID"
     << setw(40) << "NAME"
     << setw(8) << "STATE"
     << setw(15) << "RES"
     << setw(15) << "VIRT"
     << setw(10) << "CPU%"
     << endl;

    while ((entry = readdir(proc)) != nullptr){
        if (isPID(entry->d_name)){
            string pid = entry->d_name;
            string res = formatMemory(getMemory(entry->d_name,"VmRSS:"));
            string virt = formatMemory(getMemory(entry->d_name,"VmSize:"));
            string name=getProcessName(pid);
            if (name.empty())
                continue;

            p.pid = pid;
            p.name = name;
            p.state = getProcessState(pid);
            p.res = res;
            p.virt = virt;
            p.cpu = getCPUpercent(pid,oldT,newT,total1,total2);

            processes.push_back(p);
        }
    }
    closedir(proc);

    //sorting basis of cpu consumption!
    sort(processes.begin(),processes.end(),[](const ProcessInfo &a,const ProcessInfo &b){
        return a.cpu > b.cpu;
    });

    for (const ProcessInfo &p: processes){
        stringstream ss;
        ss << fixed << setprecision(2) << p.cpu << "%";
    cout << left
     << setw(8) << p.pid
     << setw(40) << p.name
     << setw(8) << p.state
     << setw(15) << p.res
     << setw(15) << p.virt
     << setw(10) << ss.str()
     << endl;
    }

    return 0;
}
