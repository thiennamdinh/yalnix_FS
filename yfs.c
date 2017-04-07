#include <comp421/iolib.h>

int main(int argc, char** argv){
    return 0;
}

int FSOpen(char *pathname){
    return 0;
}

int FSClose(int fd){
    return 0;
}

int FSCreate(char *pathname){
    return 0;
}

int FSRead(int fd, void *buf, int size){
    return 0;
}

int FSWrite(int fd, void *buf, int size){
    return 0;
}

int FSSeek(int fd, int offset, int whence){
    return 0;
}

int FSLink(char *oldname, char *newname){
    return 0;
}

int FSUnlink(char *pathname){
    return 0;
}

int FSSymLink(char *oldname, char *newname){
    return 0;
}

int FSReadLink(char *pathname, char *buf, int len){
    return 0;
}

int FSMkDir(char *pathname){
    return 0;
}

int FSRmDir(char *pathname){
    return 0;
}

int FSChDir(char *pahtname){
    return 0;
}

int FSStat(char *pathname, struct Stat*statbuf){
    return 0;
}

int FSSync(void){
    return 0;
}

int FSShutdown(void){
    return 0;
}
