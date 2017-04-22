#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

/*
 * Create empty files named "file00" through "file31" in "/".
 */
int
main()
{
	int fd;
	int i;
	char name[7];
	char* msg1 = "    Hello, world! It's a great day to be writing yalnix file systems\n";
	printf("strlen %d\n", strlen(msg1));
	
	for (i = 0; i < 1; i++) {
		sprintf(name, "file%02d", i);
		fd = Create(name);
		Close(fd);
	}
	

        for (i = 0; i < 1; i+= 2) {
	    printf("Starting file %d\n", i);
	    sprintf(name, "file%02d", i);
	    fd = Open(name);
	    char* msg0 = "    Hello, world! It's a great day to be writing yalnix file systems\n";
	    char msg[strlen(msg0) + 1];
	    memcpy(msg, msg0, strlen(msg0));
	    int j;
	    int bytes_written = 0;
	    for(j = 0; j < 25 * 512 / strlen(msg); j++){
		msg[0] = '0' + j / 100;
		msg[1] = '0' + (j % 100) / 10;
		msg[2] = '0' + j % 10;
		bytes_written += Write(fd, msg, strlen(msg));
	    }
	    printf("Bytes Written: %d\n", bytes_written);
	    Seek(fd, 0, SEEK_SET);
	    int bytes_read = 0;
	    while(1){
		int len = 13000;
		char read[len + 1];
		memset(read, '\0', len+1);
	        int result = Read(fd, read, len);
		bytes_read += result;
		//	printf("\nbytes read %d\n\n", result);
		printf("%s", read);
		if(result == 0)
		    break;
	    }
	    printf("\n");
	    printf("Bytes Read: %d\n", bytes_read);
	    Close(fd);
	}

	Shutdown();
}
