#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int main(int argc, char** argv){

    printf("Starting Test 1\n");
    
    char* dir_name1 = "/spam1";
    char* dir_name2 = "/spam1/spam2";
    char* file_name1 = "/spam1/spam2/foo1";
    char* file_name2 = "foo2";
    
    int result1 = MkDir(dir_name1);
    int result2 = MkDir(dir_name2);
    int fd1 = Create(file_name1);
    int fd2 = Create(file_name2);

    printf("result %d\n", result1);
    printf("result %d\n", result2);
    printf("result %d\n", fd1);
    printf("result %d\n", fd2);

    char* write1 = "Hello, world!\n";
    Write(fd1, write1, strlen(write1));
    
    Close(fd1);
    int fd1B = Open(file_name1);

    char read1[100];
    Read(fd1B, read1, 5);
    printf("File contents %s\n", read1);
    Shutdown();
    return 0;
}
