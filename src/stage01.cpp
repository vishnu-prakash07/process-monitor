#include <iostream>
#include <dirent.h>

using namespace std;

int main(){
    DIR* proc=opendir("/proc");
    if (proc == nullptr){
        cout << "Failed to open directory!" << endl;
        return 1;
    }
    dirent* entry ;
    while ((entry = readdir(proc)) != nullptr)
        cout << entry->d_name <<  endl;
    closedir(proc);

    return 0;
}
