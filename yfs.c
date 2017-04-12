#include "yfs.h"

// note that file system must handle "holes" in block caused by seek ahead of file
// all YFS call return error status except:
//    open and create returns the inode number
//    read and write return the number of bytes read or written
//    seek returns the size of the file


block_wrap *block_head, *block_tail;

inode_wrap *inode_head, *inode_tail;

block_wrap* get_lru_block() {

}

int put_lru_block() {

}

block_wrap* get_lru_inode() {

}

int put_lru_inode() {

}

int FSOpen(char *pathname){
    printf("called FSOpen\n");
    return 0;
}

int FSCreate(char *pathname){
    printf("called FSCreate\n");
    return 0;
}

int FSRead(void *buf, int size, short inode, int position){
    printf("called FSRead\n");
    return 0;
}

int FSWrite(void *buf, int size, short inode, int position){
    printf("called FSWrite\n");
    return 0;
}

int FSSeek(short inode){
    printf("called FSSeek\n");
    return 0;
}

int FSLink(char *oldname, char *newname){
    printf("called FSLink\n");
    return 0;
}

int FSUnlink(char *pathname){
    printf("called FSUnlink\n");
    return 0;
}

int FSSymLink(char *oldname, char *newname){
    printf("called FSSymLink\n");
    return 0;
}

int FSReadLink(char *pathname, char *buf, int len){
    printf("called FSReadLink\n");
    return 0;
}

int FSMkDir(char *pathname){
    printf("called FSMkDir\n");
    return 0;
}

int FSRmDir(char *pathname){
    printf("called FSRmDir\n");
    return 0;
}

int FSChDir(char *pahtname){
    printf("called FSChDir\n");
    return 0;
}

int FSStat(char *pathname, struct Stat*statbuf){
    printf("called FSStat\n");
    return 0;
}

int FSSync(void){
    printf("called FSSync\n");
    return 0;
}

int FSShutdown(void){
    printf("called FSShutdown\n");
    return 0;
}


int main(int argc, char** argv){

#pragma GCC diagnostic push /*ignore unavoidable gcc warning caused by unconventional casting */
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"

    int pid = Fork();
    if(pid == 0){
	char* argvec[2];
	argvec[0] = "init";
	argvec[1] = "\n";
	Exec(argvec[0], argvec);
    }

    Register(FILE_SERVER);

    // stand by and simply route messages from here on out
    while(1){
	char msg[MESSAGE_SIZE];
	int pid = Receive(msg);
	if(pid == -1){
	    fprintf(stderr, "Receive() returned error\n");
	    continue;
	}
	if(pid == 0){
	    fprintf(stderr, "Recieve() returned 0 to avoid deadlock\n");
	    continue;
	}

	int offset[4];
	uint8_t code = (uint8_t)(msg[0]);
	offset[0] = sizeof(code);
	int result;


	switch(code){
	case CODE_OPEN:
	    result = FSOpen((char*)msg[offset[0]]);
            break;
	case CODE_CREATE:
	    result = FSCreate((char*)msg[offset[0]]);
            break;
	case CODE_READ:
	    offset[1] = offset[0] + sizeof(void*);
	    offset[2] = offset[1] + sizeof(int);
	    offset[3] = offset[2] + sizeof(short);
	    result = FSRead((void*)msg[offset[0]], (int)msg[offset[1]], (short)msg[offset[2]],
			    (int)msg[offset[3]]);
	    break;
	case CODE_WRITE:
	    offset[1] = offset[0] + sizeof(void*);
	    offset[2] = offset[1] + sizeof(int);
	    offset[3] = offset[2] + sizeof(short);
	    result = FSWrite((void*)msg[offset[0]], (int)msg[offset[1]], (short)msg[offset[2]],
			    (int)msg[offset[3]]);
	    break;
	case CODE_SEEK:
	    result = FSSeek((short)msg[offset[0]]);
	    break;
	case CODE_LINK:
	    offset[1] = offset[0] + sizeof(char*);
	    result = FSLink((char*)msg[offset[0]], (char*)msg[offset[1]]);
	    break;
	case CODE_UNLINK:
	    result = FSUnlink((char*)msg[offset[0]]);
	    break;
	case CODE_SYMLINK:
	    offset[1] = offset[0] + sizeof(char*);
	    result = FSSymLink((char*)msg[offset[0]], (char*)msg[offset[1]]);
	    break;
	case CODE_READLINK:
	    offset[1] = offset[0] + sizeof(char*);
	    offset[2] = offset[1] + sizeof(char*);
	    result = FSReadLink((char*)msg[offset[0]], (char*)msg[offset[1]], (int)msg[offset[2]]);
	    break;
	case CODE_MKDIR:
	    result = FSMkDir((char*)msg[offset[0]]);
	    break;
	case CODE_RMDIR:
	    result = FSRmDir((char*)msg[offset[0]]);
	    break;
	case CODE_CHDIR:
	    result = FSChDir((char*)msg[offset[0]]);
	    break;
	case CODE_STAT:
	    offset[1] = offset[0] + sizeof(char*);
	    result = FSStat((char*)msg[offset[0]], (struct Stat*)msg[offset[1]]);
	    break;
	case CODE_SYNC:
	    result = FSSync();
	    break;
	case CODE_SHUTDOWN:
	    result = FSShutdown();
	    break;
	default:
	    result = ERROR;
	}
	
	// clean msg
	int i;
	for(i = 0; i < MESSAGE_SIZE; i++){
	    msg[i] = '\0';
	}

	// copy in result and reply
	memcpy(msg, &result, sizeof(result));
	Reply(msg, pid);
      
    }

    return 0;

#pragma GCC diagnostic pop /*pop -Wint-to-pointer-cast ignore warning*/
}
