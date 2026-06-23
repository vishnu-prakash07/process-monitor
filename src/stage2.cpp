#include <iostream>
#include <dirent.h>
#include <cctype>

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

int main(){
    
    DIR* proc=opendir("/proc");
    
    if (proc == nullptr){
        cout << "Failed to open directory!" << endl;
        return 1;
    }

    dirent* entry ;
    
    while ((entry = readdir(proc)) != nullptr){
        if (isPID(entry->d_name))
            cout << entry->d_name <<  endl;
    }

    closedir(proc);

    return 0;
}
