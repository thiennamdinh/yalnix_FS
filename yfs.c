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
    
    printf("called FSOpen %s\n", pathname);
    
    return 10;
}

int FSCreate(char *pathname){
    printf("called FSCreate %s\n", pathname);
    return 11;
}

int FSRead(void *buf, int size, short inode, int position){
    printf("called FSRead %s %d %d %d\n", buf, size, inode, position);
    char* contents = "These are contents from the read operation.";
    memcpy(buf, contents, strlen(contents) + 1);
    return 0;
}

int FSWrite(void *buf, int size, short inode, int position){
    printf("called FSWrite %s %d %d\n", buf, size, inode, position);
    printf("writing contents %s\n", buf);
    return 0;
}

int FSSeek(short inode){
    printf("called FSSeek %d\n", inode);
    return 500;
}

int FSLink(char *oldname, char *newname){
    printf("called FSLink %s %s\n", oldname, oldname);
    return 0;
}

int FSUnlink(char *pathname){
    printf("called FSUnlink %s\n", pathname);
    return 0;
}

int FSSymLink(char *oldname, char *newname){
    printf("called FSSymLink %s %s\n", oldname, newname);
    return 0;
}

int FSReadLink(char *pathname, char *buf, int len){
    printf("called FSReadLink %s %s %d\n", pathname, buf, len);
    char* link = "foo/spam/reading_link";
    memcpy(buf, link, strlen(link));
    return 0;
}

int FSMkDir(char *pathname){
    printf("called FSMkDir %s\n", pathname);
    return 0;
}

int FSRmDir(char *pathname){
    printf("called FSRmDir %s\n", pathname);
    return 0;
}

int FSChDir(char *pathname){
    printf("called FSChDir %s\n", pathname);
    return 0;
}

int FSStat(char *pathname, struct Stat* statbuf){
    printf("called FSStat %s %d\n", pathname, statbuf);
    statbuf->inum = 20;
    statbuf->type = 21;
    statbuf->size = 22;
    statbuf->nlink = 23;
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
	char* current = msg;
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
	    case CODE_OPEN:{
		char* upathname; int upathname_size;
		memcpy(&upathname, current += sizeof(code), sizeof(upathname));
		memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));

		char* pathname = (char*)malloc(upathname_size + 1);
	        CopyFrom(pid, pathname, upathname, upathname_size + 1);
		result = FSOpen(pathname);
		free(pathname);
		break;
	    }
	    case CODE_CREATE:{
		char* upathname; int upathname_size;
		memcpy(&upathname, current += sizeof(code), sizeof(upathname));
		memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));

		char* pathname = (char*)malloc(upathname_size + 1);
	        CopyFrom(pid, pathname, upathname, upathname_size + 1);
		result = FSCreate(pathname);
		free(pathname);
		break;
	    }
	    case CODE_READ:{
		char* ubuf; int usize; short uinode; int uposition;
		memcpy(&ubuf, current += sizeof(code), sizeof(ubuf));
		memcpy(&usize, current += sizeof(ubuf), sizeof(usize));
		memcpy(&uinode, current += sizeof(usize), sizeof(uinode));
		memcpy(&uposition, current += sizeof(uinode), sizeof(uposition));

		char* buf = malloc(usize + 1);
		result = FSRead(buf, usize, uinode, uposition);
		CopyTo(pid, ubuf, buf, usize + 1);
		free(buf);
		break;
	    }
	    case CODE_WRITE:{
		char* ubuf; int usize; short uinode; int uposition;
		memcpy(&ubuf, current += sizeof(code), sizeof(ubuf));
		memcpy(&usize, current += sizeof(ubuf), sizeof(usize));
		memcpy(&uinode, current += sizeof(usize), sizeof(uinode));
		memcpy(&uposition, current += sizeof(uinode), sizeof(uposition));

		char* buf = malloc(usize + 1);
		CopyFrom(pid, buf, ubuf, usize + 1);
		result = FSWrite(buf, usize, uinode, uposition);
		free(buf);
		break;
	    }
	    case CODE_SEEK:{
		short inode;	    
		memcpy(&inode, current += sizeof(code), sizeof(inode));
		result = FSSeek(inode);
		break;
	    }
	    case CODE_LINK:{
 		char* uoldname; int uoldname_size; char* unewname; int unewname_size;
		memcpy(&uoldname, current += sizeof(code), sizeof(uoldname));
		memcpy(&uoldname_size, current += sizeof(uoldname), sizeof(uoldname_size));
		memcpy(&unewname, current += sizeof(unewname_size), sizeof(unewname));
		memcpy(&unewname_size, current += sizeof(unewname), sizeof(unewname_size));

		char* oldname = malloc(uoldname_size + 1);
		char* newname = malloc(unewname_size + 1);
		CopyFrom(pid, oldname, uoldname, uoldname_size + 1);
		CopyFrom(pid, newname, unewname, unewname_size + 1);
		result = FSLink(oldname, newname);
		break;
	    }
	    case CODE_UNLINK:{
		char* upathname; int upathname_size;
		memcpy(&upathname, current += sizeof(code), sizeof(upathname));
		memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));

		char* pathname = (char*)malloc(upathname_size + 1);
	        CopyFrom(pid, pathname, upathname, upathname_size + 1);
		result = FSUnlink(pathname);
		free(pathname);
		break;
	    }
	    case CODE_SYMLINK:{
 		char* uoldname; int uoldname_size; char* unewname; int unewname_size;
		memcpy(&uoldname, current += sizeof(code), sizeof(uoldname));
		memcpy(&uoldname_size, current += sizeof(uoldname), sizeof(uoldname_size));
		memcpy(&unewname, current += sizeof(unewname_size), sizeof(unewname));
		memcpy(&unewname_size, current += sizeof(unewname), sizeof(unewname_size));

		char* oldname = malloc(uoldname_size + 1);
		char* newname = malloc(unewname_size + 1);
		CopyFrom(pid, oldname, uoldname, uoldname_size + 1);
		CopyFrom(pid, newname, unewname, unewname_size + 1);
		result = FSSymLink(oldname, newname);
		break;
	    }
	    case CODE_READLINK:{
		char* upathname; int upathname_size; char* ubuf; int ulen;
		memcpy(&upathname, current += sizeof(code), sizeof(upathname));
		memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
		memcpy(&ubuf, current += sizeof(upathname_size), sizeof(ubuf));
		memcpy(&ulen, current += sizeof(ubuf), sizeof(ulen));
		printf("path size %d len %d\n", upathname_size, ulen);
		char* pathname = malloc(upathname_size + 1);
		char* buf = malloc(ulen + 1);
		CopyFrom(pid, pathname, upathname, upathname_size + 1);
	        result = FSReadLink(pathname, buf, ulen);
		CopyTo(pid, ubuf, buf, ulen + 1);
	        free(pathname);
		free(buf);
		break;
	    }
	    case CODE_MKDIR:{
		char* upathname; int upathname_size;
		memcpy(&upathname, current += sizeof(code), sizeof(upathname));
		memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));

		char* pathname = (char*)malloc(upathname_size + 1);
	        CopyFrom(pid, pathname, upathname, upathname_size + 1);
		result = FSMkDir(pathname);
		free(pathname);
		break;
	    }
	    case CODE_RMDIR:{
		char* upathname; int upathname_size;
		memcpy(&upathname, current += sizeof(code), sizeof(upathname));
		memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));

		char* pathname = (char*)malloc(upathname_size + 1);
	        CopyFrom(pid, pathname, upathname, upathname_size + 1);
		result = FSRmDir(pathname);
		free(pathname);
		break;
	    }
	    case CODE_CHDIR:{
		char* upathname; int upathname_size;
		memcpy(&upathname, current += sizeof(code), sizeof(upathname));
		memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));

		char* pathname = (char*)malloc(upathname_size + 1);
	        CopyFrom(pid, pathname, upathname, upathname_size + 1);
		result = FSChDir(pathname);
		free(pathname);
		break;
	    }
	    case CODE_STAT:{
		char* upathname; int upathname_size; struct Stat* ustatbuf;
		memcpy(&upathname, current += sizeof(code), sizeof(upathname));
		memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
		memcpy(&ustatbuf, current += sizeof(upathname_size), sizeof(ustatbuf));

		char* pathname = (char*)malloc(upathname_size + 1);
		char* statbuf = malloc(sizeof(struct Stat));
	        CopyFrom(pid, pathname, upathname, upathname_size + 1);
		result = FSStat(pathname, statbuf);
		CopyTo(pid, ustatbuf, statbuf, sizeof(statbuf));
		free(pathname);
		free(statbuf);
		break;
	    }
	    case CODE_SYNC:{
		result = FSSync();
		break;
	    }
	    case CODE_SHUTDOWN:{
		result = FSShutdown();
		break;
	    }
	    default:{
		result = ERROR;
	    }

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
