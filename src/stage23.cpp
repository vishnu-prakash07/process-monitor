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

    long ppid;
    //sorting of res
    long resKB;
    long pidSort;
};

struct MemInfo{
    long totalMem;
    long availableMem;
};

struct LoadInfo{
    double load1;
    double load5;
    double load15;
};

struct CPUCores{
    long total;
    long idle;
};

const string RED    = "\033[31m";
const string GREEN  = "\033[32m";
const string YELLOW = "\033[33m";
const string RESET  = "\033[0m";

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

MemInfo getRAMUsage(){
    MemInfo ram;
    ifstream file("/proc/meminfo");

    string label;
    long value;
    string unit;

    while (file >> label >> value >> unit){
        if (label == "MemTotal:")
            ram.totalMem = value;
        else if (label == "MemAvailable:")
            ram.availableMem = value;
    }

    return ram;
}

double convertToDouble(long kb){
    return kb/(1024.0 * 1024.0);
}

long getCPUInfo(){
    ifstream file("/proc/stat");
    string line;
    getline(file,line);
    stringstream ss(line);

    string cpu;
    ss >> cpu;

    long value;
    int count=0;
    long idle=0;

    while (ss >> value){
        if (count == 3 || count == 4){ // user nice system idle iowait
            idle += value; // idleTime in linux is idle+iowait!
        }
        count ++;
    }

    return idle;
}

double findCPUpercent(long idle1,long idle2,long total1,long total2){
    double cpuPercent ;
    long deltaIdle = idle2 - idle1;
    long deltaTotal = total2 - total1;
    
    if (deltaTotal <= 0)
        return 0.0;
    
    long deltaBusy = deltaTotal - deltaIdle; //busyTime = TotalCPUTime - TotalIdleTime;

    cpuPercent = (double) (deltaBusy * 100.0) / deltaTotal ;

    return cpuPercent;
}

int getProcessCount(){
    DIR *path = opendir("/proc");

    int count = 0;

    if (path == nullptr){
        cout << "No process Directory!" << endl;
        return 0;
    }

    dirent *entry;

    while ((entry = readdir(path)) != nullptr){
        if (isPID(entry -> d_name))
            count ++;
    }

    return count;
}

long getUpTime(){
    ifstream file("/proc/uptime");
    long uptime ;

    file >> uptime ; //[uptime(in seconds)] [cpuidleTIme(since boot)]

    return uptime;
}

string formatUptime(long uptime){
    
    long days = uptime / 86400 ; //60*60*24
    long hours = (uptime % 86400) / 3600 ; //60*60
    long minutes = (uptime % 3600) / 60 ; 

    stringstream ss;

    ss << days << "d " << hours << "h " << minutes << "m";

    return ss.str();
}

string getCPUColor(double cpu)
{
    if(cpu >= 10)
        return RED;

    if(cpu >= 5)
        return YELLOW;

    return GREEN;
}

string getStateColor(char S){
    if (S == 'R')
        return GREEN;
    else if (S == 'Z')
        return RED;
    else if (S == 'D' || S == 'I') //Disk idle or idle
        return YELLOW;
    
    return RESET;
}

LoadInfo getLoadInfo(){
    LoadInfo load;
    ifstream file("/proc/loadavg");
    double load1,load5,load15;

    file >> load1;
    file >> load5;
    file >> load15;

    load.load1 = load1;
    load.load5 = load5;
    load.load15 = load15;

    return load;
}
vector <CPUCores> getCPUCore(){
    vector <CPUCores> CPU;
    CPUCores temp;
    
    ifstream file("/proc/stat");
    string line ;
    stringstream ss;
    getline(file,line);

    string cpu;

    long value;
    int count = 0;
    long idle = 0;
    long total = 0;
    int index = 0;

    while (index < 8){
        getline (file,line);
        stringstream ss(line);
        idle = 0;
        total = 0;
        count = 0;
        ss >> cpu; //label
        while (ss >> value){
            if (count == 3 || count == 4){ // user nice system idle iowait
                idle += value; // idleTime in linux is idle+iowait!
            }
            total += value;
            count ++;
        }
        temp.idle = idle;
        temp.total = total;
        CPU.push_back(temp);
        index++;
    }

    return CPU;
}

vector<double> findCPUCorePercent(vector<CPUCores> Snap1 , vector<CPUCores> Snap2){
    vector <double> answer;
    double cpuPercent ;

    long deltaIdle, deltaTotal, deltaBusy;

    for (int i=0;i<8;i++){

        deltaTotal = Snap2[i].total - Snap1[i].total;
        deltaIdle = Snap2[i].idle - Snap1[i].idle;

        if (deltaTotal <= 0){
            answer.push_back(0.0);
            continue;
        }

        deltaBusy = deltaTotal - deltaIdle; //busyTime = TotalCPUTime - TotalIdleTime;
        cpuPercent = (double) (deltaBusy * 100.0) / deltaTotal ;

        answer.push_back(cpuPercent);
    }

    return answer;
}

string makeBar(double percent){
    int width = 20;
    int filled = ( width * percent ) / 100;

    string bar = "[";

    for (int i = 0; i < width; i++){
        if (i < filled)
            bar += "#";
        else
            bar += "-";
    }
    bar += "]";

    return bar;
}

string getBarColor(double percent){
    if (percent >= 60)
        return RED;
    if (percent >= 25)
        return YELLOW;
    else    
        return GREEN;
}

long getPPID(const string &pid){
    string path = "/proc/" + pid + "/status";

    ifstream file(path);
    string line;

    while (getline(file,line)){
        if (line.find("PPid:") == 0){
            string label;
            long ppid;

            stringstream ss(line);

            ss >> label >> ppid;

            return ppid;
        }
    }
    return -1;
}

void printTree(long pid,const unordered_map<long,vector<long>> &children,const unordered_map<long,ProcessInfo> &processMap, int depth = 0){
    
    if (processMap.find(pid) == processMap.end()) //its empty
        return;
    
    //intendation
    for (int i=0 ; i<depth ; i++){
        if (i == (depth-1))
            cout << "|__";
        else
            cout << "  ";
    }

   cout << processMap.at(pid).name << "(" << pid << ")" << endl;

    auto it = children.find(pid); // for const unordered_map if map.at(pid) is not exist then it should create an empty child and it will gives rise to a out of range error!

    if(it == children.end())
        return;

    for(long child : it->second)
    {
        printTree(child, children, processMap, depth + 1);
    }
}

string makeSparkLine(vector<double> &CPUHistory){
   vector<string> levels =
{
    "▁",
    "▂",
    "▃",
    "▄",
    "▅",
    "▆",
    "▇",
    "█"
}; //7 indices!

    string graph;

    for (double value : CPUHistory){
        int index = (value * 7) / 100 ;

        if (index < 0)
            index = 0;
        if (index > 7)
            index = 7;
        
        graph += levels[index];
    }
    return graph;
}

int main(int argc, char* argv[]){

    string sortMode = "cpu";
    string filter = "";

    if(argc > 1)
    {
        sortMode = argv[1];

        if(sortMode != "cpu" &&
           sortMode != "mem" &&
           sortMode != "pid" &&
           sortMode != "name" &&
            sortMode != "tree")
        {
            cout << "Invalid sort mode!\n";
            cout << "Usage: ./monitor [cpu|mem|pid|name|tree]\n";
            return 1;
        }
    }

    if (argc > 2)
        filter = argv[2];

    //stage 23 - cpu history graph

    vector <double> CPUHistory;

    while (true){

        DIR* proc = opendir("/proc");
    
        if (proc == nullptr){
            cout << "Failed to open directory!" << endl;
            return 1;
        }

        dirent* entry ;

        //stage 17
        LoadInfo loadAvg = getLoadInfo();
        
        //stage 12
        MemInfo ram = getRAMUsage();
        double totalRAM = convertToDouble(ram.totalMem);
        double usedRAM = totalRAM - convertToDouble(ram.availableMem);

        auto oldT = getSnapshot();
        long total1 = getTotalCPUTime();
        long idle1 = getCPUInfo();
        auto CPUSnap1 = getCPUCore();

        sleep(1);

        auto newT = getSnapshot();
        long total2 = getTotalCPUTime();
        long idle2 = getCPUInfo();
        auto CPUSnap2 = getCPUCore();

        double CPUpercent = findCPUpercent(idle1,idle2,total1,total2);
        CPUHistory.push_back(CPUpercent);

        if (CPUHistory.size() > 20){ //keeping only 20 and remove from front
            CPUHistory.erase(CPUHistory.begin());
        }

        string CPUBar = makeBar(CPUpercent);

        double RAMpercent = ((double) usedRAM * 100.0) / totalRAM ;
        string RAMBar = makeBar(RAMpercent);

        vector <double> CoresCPU = findCPUCorePercent(CPUSnap1,CPUSnap2);
        
        vector <ProcessInfo> processes;

        //stage 22
        unordered_map <long,vector<long>> children;
        unordered_map <long,ProcessInfo> processMap;

        long upTime = getUpTime();

        system("clear");
        // cout << "\033[H\033[2J"; //--> ANSI code for moving cursor Home

        string levels = makeSparkLine(CPUHistory);
        
        cout << "Process Monitor (refresh: 1 sec)\n";
        cout << "Sort: " << sortMode << endl ;
        if(!filter.empty())
            cout << "Filter: " << filter << endl << endl;
        cout << fixed << setprecision(2);

        cout << "==============================================\n";
        cout << "CPU: " << getBarColor(CPUpercent) << CPUBar << " " << RESET << CPUpercent << "%" << endl;
        cout << "History: " << makeSparkLine(CPUHistory) << endl;
        for (int i=0;i<8;i++){
            cout << "CPU" << i << ": " << CoresCPU[i] << "%" << endl;
        }
        cout << endl;
        cout << "RAM: " << getBarColor(RAMpercent) << RAMBar << " " << RESET << usedRAM << " GB / " << totalRAM << " GB\n";
        cout << "Load: " << loadAvg.load1 << " " << loadAvg.load5 << " " << loadAvg.load15 << " " << endl;
        cout << "Processes: " << getProcessCount() << "\n";
        cout << "Uptime: " << formatUptime(upTime) << "\n";
        cout << "==============================================\n\n";

        if (sortMode != "tree"){
            cout << left
            << setw(8) << "PID"
            << setw(40) << "NAME"
            << setw(8) << "STATE"
            << setw(15) << "RES"
            << setw(15) << "VIRT"
            << setw(10) << "CPU%"
            << endl;
        }

        while ((entry = readdir(proc)) != nullptr){
            if (isPID(entry->d_name)){
                string pid = entry->d_name;
                long pidSort = stol(pid);
                long ppid = getPPID(pid);
                long resKB = getMemory(entry->d_name,"VmRSS:");
                string res = formatMemory(resKB);
                string virt = formatMemory(getMemory(entry->d_name,"VmSize:"));
                string name=getProcessName(pid);
                if (name.empty())
                    continue;
                
                //filter
                if (filter.empty() == 0){ //not empty
                    if (name.find(filter) == string::npos) //if not present
                        continue;   
                }
                ProcessInfo p;

                p.pid = pid;
                p.name = name;
                p.state = getProcessState(pid);
                p.res = res;
                p.virt = virt;
                p.cpu = getCPUpercent(pid,oldT,newT,total1,total2);
                p.resKB = resKB;
                p.pidSort = pidSort;
                p.ppid = ppid;

                processes.push_back(p);
                children[ppid].push_back(pidSort); // if 100,101,102 are the children of 1 then children[1] ={100,101,102}
                processMap[pidSort] = p; // easily track processes with pid itself!
            }
        }
        closedir(proc);

        //sorting basis of cpu consumption!
        //descending sorting using lambda method!
        
            //tree printing!
        if (sortMode == "tree")
            printTree(1,children,processMap,0);

        else if(sortMode == "mem")
            sort(processes.begin(),processes.end(),[](const ProcessInfo &a,const ProcessInfo &b){
                return a.resKB > b.resKB;
            });
        
        else if (sortMode == "pid")
            sort(processes.begin(),processes.end(),[](const ProcessInfo &a,const ProcessInfo &b){
                return a.pidSort > b.pidSort;
            });
        
        else if (sortMode == "name")
            sort(processes.begin(),processes.end(),[](const ProcessInfo &a,const ProcessInfo &b){
                return a.name < b.name;
            });
        else if (sortMode == "cpu")
            sort(processes.begin(),processes.end(),[](const ProcessInfo &a,const ProcessInfo &b){
                return a.cpu > b.cpu;
            });

        if (sortMode != "tree"){    
            for (int i=0;i< min(20,(int)processes.size());i++){
                stringstream ss;
                ProcessInfo p;
                p = processes[i];

                string CPUcolor = getCPUColor(p.cpu);
                string StateColor = getStateColor(p.state);

                ss << fixed << setprecision(2) << p.cpu << "%";
            cout << left
            << setw(8) << p.pid
            << setw(40) << p.name << StateColor 
            << setw(8) << p.state << RESET
            << setw(15) << p.res
            << setw(15) << p.virt;

            cout << CPUcolor << setw(10) << ss.str() << RESET << endl;
            }
        }
        //sleep(10);
    }

    return 0;
}