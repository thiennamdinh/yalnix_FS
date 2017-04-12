#include <stdio.h>
#include <stdlib.h>

#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int main(int argc, char** argv){

    int buflen = 100;
    char buf[buflen];
    char* pathname1 = "/home/thien-nam/Documents";
    char* pathname2 = "/usr/bin";
    int fd;
    struct Stat stat;
    
    printf("got here %d\n", 1);
    fd = Open(pathname1);
    printf("got here %d\n", 2);
    Close(fd);
    printf("got here %d\n", 3);
    fd = Create(pathname1);
    printf("got here %d\n", 4);
    Read(1, buf, buflen);
    printf("got here %d\n", 5);
    Write(1, buf, buflen);
    printf("got here %d\n", 6);
    Seek(1, 10, SEEK_SET);
    printf("got here %d\n", 7);
    Link(pathname1, pathname2);
    printf("got here %d\n", 8);
    Unlink(pathname2);
    printf("got here %d\n", 9);
    SymLink(pathname1, pathname2);
    printf("got here %d\n", 10);
    ReadLink(pathname1, buf, buflen);
    printf("got here %d\n", 11);
    MkDir(pathname1);
    printf("got here %d\n", 12);
    RmDir(pathname1);
    printf("got here %d\n", 13);
    ChDir(pathname1);
    printf("got here %d\n", 14);
    Stat(pathname1, &stat);
    printf("got here %d\n", 15);
    Sync();
    printf("got here %d\n", 16);
    Shutdown();
    
    while(1){
	Pause();
	printf("idling\n");
    }
    return 0;
}
