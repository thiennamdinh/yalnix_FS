#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int main(int argc, char** argv){

    printf("Starting Test 2\n");

    int fd = Create("foo1");
    
    char* msg1 = "Hello, world!\n";
    char* msg2 = "This should not be overwriting!\n";
    
    Write(fd, msg1, strlen(msg1));
    Write(fd, msg2, strlen(msg2));
    
    char readbuf[100];
    Seek(fd, 0, SEEK_SET);
    int result = Read(fd, readbuf, 100);
    printf("Read %d: %s\n", result, readbuf);
    Shutdown();
    return 0;
}
