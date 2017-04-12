#include "yfs.h"

struct Open_File {
    short inode;
    int position;
};

struct Open_File files[MAX_OPEN_FILES];
int num_open_files = 0;

int CallYFS(uint8_t code, void** args, int* arg_sizes, int num_args){
    char msg[MESSAGE_SIZE];
    msg[0] = (char)code;

    int i;
    int offset = 1;
    for(i = 0; i < num_args; i++){
	// no longs are passed so sizeof(void*) applies exclusively to pointers
        if(arg_sizes[i] == sizeof(void*) && *(void**)args[i] == NULL){
	    printf("ERROR: null pointer argument provided\n");
	    return ERROR;
	}
	memcpy(msg + offset, (char*)(args[i]), arg_sizes[i]);
	offset += arg_sizes[i];
    }
 
    Send(msg, -FILE_SYSTEM);

    //----------------------------------------------------------------------------------------
    // pass control to YFS; upon return, msg should be overwritten with single int reply value
    //----------------------------------------------------------------------------------------

    int result = *(int*)msg;
    if(result == -1){
	switch(code){
	case CODE_OPEN:
	    fprintf(stderr, "ERROR: YFS failed to open file\n"); break;
	case CODE_CLOSE:
	    fprintf(stderr, "ERROR: YFS failed to close file\n"); break;
	case CODE_CREATE:
	    fprintf(stderr, "ERROR: YFS failed to create file\n"); break;
	case CODE_READ:
	    fprintf(stderr, "ERROR: YFS failed to read file\n"); break;
	case CODE_WRITE:
	    fprintf(stderr, "ERROR: YFS failed to write to file\n"); break;
	case CODE_SEEK:
	    fprintf(stderr, "ERROR: YFS failed to seek file\n"); break;
	case CODE_LINK:
	    fprintf(stderr, "ERROR: YFS failed to create link\n"); break;
	case CODE_UNLINK:
	    fprintf(stderr, "ERROR: YFS failed to unlink path\n"); break;
	case CODE_SYMLINK:
	    fprintf(stderr, "ERROR: YFS failed to create symbolic link\n"); break;
	case CODE_READLINK:
	    fprintf(stderr, "ERROR: YFS failed to read symbolic link\n"); break;
	case CODE_MKDIR:
	    fprintf(stderr, "ERROR: YFS failed to make directory\n"); break;
	case CODE_RMDIR:
	    fprintf(stderr, "ERROR: YFS failed to remove directory\n"); break;
	case CODE_CHDIR:
	    fprintf(stderr, "ERROR: YFS failed to change directory\n"); break;
	case CODE_STAT:
	    fprintf(stderr, "ERROR: YFS failed to retrieve stats\n"); break;
	case CODE_SYNC:
	    fprintf(stderr, "ERROR: YFS failed to sync storage\n"); break;
	case CODE_SHUTDOWN:
	    fprintf(stderr, "ERROR: YFS failed to shutdown\n"); break;
	}
    }
    return result;
}

int Open(char* pathname){

    if(num_open_files >= MAX_OPEN_FILES){
	fprintf(stderr, "ERROR: maximum number of files already open\n");
	return 0;
    }

    void* args[1] = {(void*)&pathname};
    int arg_sizes[1] = {sizeof(pathname)};
    int inode = CallYFS(CODE_OPEN, args, arg_sizes, 1);

    int i;
    for(i = 0; i < MAX_OPEN_FILES; i++){
	if(files[i].inode == -1){
	    files[i].inode = inode;
	    num_open_files++;
	    return i;
	}
    }

    // no reason this should ever occur since we already check max open files
    return ERROR;
}

int Close(int fd){

    if(fd < 0 || fd >= MAX_OPEN_FILES || files[fd].inode == -1){
	fprintf(stderr, "ERROR: file descriptor is invalid\n");
	return ERROR;
    }

    files[fd].inode = -1;
    files[fd].position = 0;
    return 0;
}

int Create(char *pathname){

    if(num_open_files >= MAX_OPEN_FILES){
	fprintf(stderr, "ERROR: maximum number of files already open\n");
	return 0;
    }

    void* args[1] = {(void*)&pathname};
    int arg_sizes[1] = {sizeof(pathname)};
    int inode = CallYFS(CODE_CREATE, args, arg_sizes, 1);

    int i;
    for(i = 0; i < MAX_OPEN_FILES; i++){
	if(files[i].inode == -1){
	    files[i].inode = inode;
	    num_open_files++;
	    return i;
	}
    }

    // no reason this should ever occur since we already check max open files
    return ERROR;

}

int Read(int fd, void *buf, int size){

    if(fd < 0 || fd >= MAX_OPEN_FILES || files[fd].inode == -1){
	fprintf(stderr, "ERROR: file descriptor is invalid\n");
	return ERROR;
    }

    short inode = files[fd].inode;
    int position = files[fd].position;

    void* args[4] = {(void*)&buf, (void*)&size, (void*)&inode, (void*)&position};
    int arg_sizes[4] = {sizeof(buf), sizeof(size), sizeof(inode), sizeof(position)};
    int result = CallYFS(CODE_READ, args, arg_sizes, 4);

    if(result == -1)
	return ERROR;

    files[fd].position += result;
    return result;
}

int Write(int fd, void *buf, int size){
    
    if(fd < 0 || fd >= MAX_OPEN_FILES || files[fd].inode == -1){
	fprintf(stderr, "ERROR: file descriptor is invalid\n");
	return ERROR;
    }

    short inode = files[fd].inode;
    int position = files[fd].position;

    void* args[4] = {(void*)&buf, (void*)&size, (void*)&inode, (void*)&position};
    int arg_sizes[4] = {sizeof(buf), sizeof(size), sizeof(inode), sizeof(position)};
    int result = CallYFS(CODE_WRITE, args, arg_sizes, 4);

    if(result == -1)
	return ERROR;
    
    files[fd].position += result;
    return result;
}

int Seek(int fd, int offset, int whence){

    if(fd < 0 || fd >= MAX_OPEN_FILES || files[fd].inode == -1){
	fprintf(stderr, "ERROR: file descriptor is invalid\n");
	return ERROR;
    }

    int new_position;

    switch(whence){
    case SEEK_SET:
	new_position = offset;
	break;
    case SEEK_CUR:
	new_position = files[fd].position + offset;
	break;
    case SEEK_END:
	; // fix a dumb "declaration after statement" C quirk
	int size = sizeof(files[fd].inode);
	new_position = CallYFS(CODE_SEEK, (void*)&files[fd].inode, &size, 1) - offset;
	break;
    default:
	fprintf(stderr, "ERROR: whence value invalid\n");
    }

    if(new_position < 0){
	fprintf(stderr, "ERROR: new file position would be negative\n");
	return ERROR;
    }

    files[fd].position = new_position;
    return 0;
}

int Link(char *oldname, char *newname){
    void* args[2] = {(void*)&oldname, (void*)&newname};
    int arg_sizes[2] = {sizeof(oldname), sizeof(newname)};
    return CallYFS(CODE_LINK, args, arg_sizes, 2);
}

int Unlink(char *pathname){
    void* args[1] = {(void*)&pathname};
    int arg_sizes[1] = {sizeof(pathname)};
    return CallYFS(CODE_UNLINK, args, arg_sizes, 1);
}
	
int SymLink(char *oldname, char *newname){
    void* args[2] = {(void*)&oldname, (void*)&newname};
    int arg_sizes[2] = {sizeof(oldname), sizeof(newname)};
    return CallYFS(CODE_SYMLINK, args, arg_sizes, 2);
}

int ReadLink(char *pathname, char *buf, int len){
    void* args[3] = {(void*)&pathname, (void*)&buf, (void*)&len};
    int arg_sizes[3] = {sizeof(pathname), sizeof(buf), sizeof(len)};
    return CallYFS(CODE_READLINK, args, arg_sizes, 3);
}

int MkDir(char *pathname){
    void* args[1] = {(void*)&pathname};
    int arg_sizes[1] = {sizeof(pathname)};
    return CallYFS(CODE_MKDIR, args, arg_sizes, 1);
}

int RmDir(char *pathname){
    void* args[1] = {(void*)&pathname};
    int arg_sizes[1] = {sizeof(pathname)};
    return CallYFS(CODE_RMDIR, args, arg_sizes, 1);
}

int ChDir(char *pathname){
    void* args[1] = {(void*)&pathname};
    int arg_sizes[1] = {sizeof(pathname)};
    return CallYFS(CODE_CHDIR, args, arg_sizes, 1);
}

int Stat(char *pathname, struct Stat* statbuf){
    void* args[2] = {(void*)&pathname, (void*)&statbuf};
    int arg_sizes[2] = {sizeof(pathname), sizeof(statbuf)};
    return CallYFS(CODE_STAT, args, arg_sizes, 2);
}

int Sync(void){
    return CallYFS(CODE_SYNC, NULL, NULL, 0);
}

int Shutdown(void){
    return CallYFS(CODE_SHUTDOWN, NULL, NULL, 0);
}
