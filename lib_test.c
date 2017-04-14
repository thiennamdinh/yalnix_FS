#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int main(int argc, char** argv){

    int buflen = 100;
    char buf[buflen];
    char* buf2 = "contents to be written";

    char* pathname1 = "/home/thien-nam/Documents";
    char* pathname2 = "/usr/bin";
    int fd;
    struct Stat stat;
    
    printf("\nopening file\n");
    fd = Open(pathname1);

    printf("\nclosing file\n");
    Close(fd);

    printf("\ncreating file\n");
    fd = Create(pathname2);
    
    printf("\nreading file\n");
    Read(fd, buf, buflen);
 
    printf("\nwriting file\n");
    Write(fd, buf2, strlen(buf2));

    printf("\nseeking file\n");
    Seek(fd, 15, SEEK_END);

    printf("\nlinking paths\n");
    Link(pathname1, pathname2);

    printf("\nunlinking path\n");
    Unlink(pathname2);

    printf("\nsymlinking paths\n");
    SymLink(pathname1, pathname2);

    printf("\nreading link\n");
    ReadLink(pathname1, buf, buflen);

    printf("\nmaking dir\n");
    MkDir(pathname1);

    printf("\nremoving dir\n");
    RmDir(pathname1);

    printf("\nchanging dir\n");
    ChDir(pathname1);

    printf("\nreading stat\n");
    Stat(pathname1, &stat);

    printf("\nsynching\n");
    Sync();

    printf("\nshutting down\n");
    Shutdown();
    
    while(1){
	Pause();
	printf("idling\n");
    }
    return 0;
}
