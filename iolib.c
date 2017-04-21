#include "yfs.h"

// TODO: clean pathnames before passing to YFS
//     take care of "//"
//     make sure less than MAXPATHNAMELEN

// TODO: enable relative path names by tracking current directory and sending inode to YFS

struct Open_File {
    short inode;
    int position;
};

struct Open_File files[MAX_OPEN_FILES];

int num_open_files = 0;
int files_initialized = 0;
short current_dir = ROOTINODE;

void Initialize_Files(){
    int i;
    for(i = 0; i < MAX_OPEN_FILES; i++){
	files[i].inode = -1;
	files[i].position = 0;
    }
    files_initialized = 1;
}

// condenses consecutive '/'s and replaces trailing '/' with '.'
// returns the length of the new pathname which is no more than strlen(pathname) + 1 
int format_pathname(char* oldname, char* newname){
    
    int len = strlen(oldname);
    int i = 0; int j = 0;
    for(i = 0; i < len; i++){
	if(oldname[i] == '/' && oldname[i+1] == '/'){
	    continue;
	}
	newname[j] = oldname[i];
	j++;
    }
    int newlen = j;

    if(newname[newlen-1] == '/'){
	newname[newlen] = '.';
	newname[newlen+1] = '\0';
	return newlen+1;
    }
    else{
	newname[newlen] = '\0';
	return newlen;
    }
}

int CallYFS(uint8_t code, void** args, int* arg_sizes, int num_args){
    char msg[MESSAGE_SIZE];
    msg[0] = (char)code;

    int i;
    int offset = 1;


    for(i = 0; i < num_args; i++){
	// no longs are passed so sizeof(void*) applies exclusively to pointers
        if(arg_sizes[i] == sizeof(void*) && *(void**)args[i] == NULL){
	    fprintf(stderr, "ERROR: null pointer argument provided\n");
	    return ERROR;
	}
	memcpy(msg + offset, (char**)(args[i]), arg_sizes[i]);
	offset += arg_sizes[i];
    }

    int success =  Send(msg, -FILE_SYSTEM);

    //----------------------------------------------------------------------------------------
    // pass control to YFS; upon return, msg should be overwritten with single int reply value
    //----------------------------------------------------------------------------------------

    int result = *(int*)msg;
    if(success == -1 || result == -1){
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
    if(!files_initialized)
	Initialize_Files();

    if(num_open_files >= MAX_OPEN_FILES){
	fprintf(stderr, "ERROR: maximum number of files already open\n");
	return 0;
    }
    
    char* tmp = pathname;
    pathname = (char*)malloc(strlen(pathname) + 2);
    int pathname_size = format_pathname(tmp, pathname);
    void* args[3] = {(void*)&pathname, (void*)&pathname_size, (void*)&current_dir};
    int arg_sizes[3] = {sizeof(pathname), sizeof(pathname_size), sizeof(current_dir)};
    int inode = CallYFS(CODE_OPEN, args, arg_sizes, 3);
    free(pathname);

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
    if(!files_initialized)
	Initialize_Files();

    if(fd < 0 || fd >= MAX_OPEN_FILES || files[fd].inode == -1){
	fprintf(stderr, "ERROR: file descriptor is invalid\n");
	return ERROR;
    }

    files[fd].inode = -1;
    files[fd].position = 0;
    num_open_files--;
    return 0;
}

int Create(char *pathname){
    if(!files_initialized)
	Initialize_Files();

    if(num_open_files >= MAX_OPEN_FILES){
	fprintf(stderr, "ERROR: maximum number of files already open\n");
	return 0;
    }


    char* tmp = pathname;
    pathname = (char*)malloc(strlen(pathname) + 2);
    int pathname_size = format_pathname(tmp, pathname);
    void* args[3] = {(void*)&pathname, (void*)&pathname_size, (void*)&current_dir};
    int arg_sizes[3] = {sizeof(pathname), sizeof(pathname_size), sizeof(current_dir)};
    int inode = CallYFS(CODE_CREATE, args, arg_sizes, 3);
    free(pathname);

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
    if(!files_initialized)
	Initialize_Files();

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
    if(!files_initialized)
	Initialize_Files();
    
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
    if(!files_initialized)
	Initialize_Files();

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
	void* args[1] = {(void*)&files[fd].inode};
	int arg_sizes[1] = {sizeof(files[fd].inode)};
        new_position = CallYFS(CODE_SEEK, args, arg_sizes, 1) - offset;
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
    
    char* tmp1 = oldname;
    oldname = (char*)malloc(strlen(oldname) + 2);
    int oldname_size = format_pathname(tmp1, oldname);
    char* tmp2 = newname;
    newname = (char*)malloc(strlen(newname) + 2);
    int newname_size = format_pathname(tmp2, newname);
    
    void* args[5] = {(void*)&oldname, (void*)&oldname_size, (void*)&newname,
		     (void*)&newname_size, (void*)&current_dir};
    int arg_sizes[5] = {sizeof(oldname), sizeof(oldname_size), sizeof(newname), 
			sizeof(newname_size), sizeof(current_dir)};
    int result = CallYFS(CODE_LINK, args, arg_sizes, 5);
    free(oldname);
    free(newname);
    return result;
}

int Unlink(char *pathname){
    char* tmp = pathname;
    pathname = (char*)malloc(strlen(pathname) + 2);
    int pathname_size = format_pathname(tmp, pathname);
    void* args[3] = {(void*)&pathname, (void*)&pathname_size, (void*)&current_dir};
    int arg_sizes[3] = {sizeof(pathname), sizeof(pathname_size), sizeof(current_dir)};
    int result =  CallYFS(CODE_UNLINK, args, arg_sizes, 3);
    free(pathname);
    return result;
}
	
int SymLink(char *oldname, char *newname){
    char* tmp1 = oldname;
    oldname = (char*)malloc(strlen(oldname) + 2);
    int oldname_size = format_pathname(tmp1, oldname);
    char* tmp2 = newname;
    newname = (char*)malloc(strlen(newname) + 2);
    int newname_size = format_pathname(tmp2, newname);
        void* args[5] = {(void*)&oldname, (void*)&oldname_size, (void*)&newname,
		     (void*)&newname_size, (void*)&current_dir};
    int arg_sizes[5] = {sizeof(oldname), sizeof(oldname_size), sizeof(newname),
			sizeof(newname_size), sizeof(current_dir)};
    int result = CallYFS(CODE_SYMLINK, args, arg_sizes, 5);
    free(oldname);
    free(newname);
    return result;
}

int ReadLink(char *pathname, char *buf, int len){
    char* tmp = pathname;
    pathname = (char*)malloc(strlen(pathname) + 2);
    int pathname_size = format_pathname(tmp, pathname);
    void* args[5] = {(void*)&pathname, (void*)&pathname_size, (void*)&buf, (void*)&len,
		     (void*)&current_dir};
    int arg_sizes[5] = {sizeof(pathname), sizeof(pathname_size), sizeof(buf), sizeof(len),
			sizeof(current_dir)};
    int result = CallYFS(CODE_READLINK, args, arg_sizes, 5);
    free(pathname);
    return result;
}

int MkDir(char *pathname){
    char* tmp = pathname;
    pathname = (char*)malloc(strlen(pathname) + 2);
    int pathname_size = format_pathname(tmp, pathname);
    void* args[3] = {(void*)&pathname, (void*)&pathname_size, (void*)&current_dir};
    int arg_sizes[3] = {sizeof(pathname), sizeof(pathname_size), sizeof(current_dir)};
    int result = CallYFS(CODE_MKDIR, args, arg_sizes, 3);
    free(pathname);
    return result;
}

int RmDir(char *pathname){
    char* tmp = pathname;
    pathname = (char*)malloc(strlen(pathname) + 2);
    int pathname_size = format_pathname(tmp, pathname);
    void* args[2] = {(void*)&pathname, (void*)&pathname_size};
    int arg_sizes[2] = {sizeof(pathname), sizeof(pathname_size)};
    int result = CallYFS(CODE_RMDIR, args, arg_sizes, 2);
    free(pathname);
    return result;
}

int ChDir(char *pathname){
    char* tmp = pathname;
    pathname = (char*)malloc(strlen(pathname) + 2);
    int pathname_size = format_pathname(tmp, pathname);
    void* args[3] = {(void*)&pathname, (void*)&pathname_size, (void*)&current_dir};
    int arg_sizes[3] = {sizeof(pathname), sizeof(pathname_size), sizeof(current_dir)};
    short new_inum = CallYFS(CODE_CHDIR, args, arg_sizes, 3);
    free(pathname);
    
    if(new_inum == ERROR){
	return ERROR;
    }
    current_dir = new_inum;
    return 0;
}

int Stat(char *pathname, struct Stat* statbuf){
    char* tmp = pathname;
    pathname = (char*)malloc(strlen(pathname) + 2);
    int pathname_size = format_pathname(tmp, pathname);
    void* args[4] = {(void*)&pathname, (void*)&pathname_size, (void*)&statbuf,
		     (void*)&current_dir};
    int arg_sizes[4] = {sizeof(pathname), sizeof(pathname_size), sizeof(statbuf),
			sizeof(current_dir)};
    int result = CallYFS(CODE_STAT, args, arg_sizes, 4);
    free(pathname);
    return result;
}

int Sync(void){
    return CallYFS(CODE_SYNC, NULL, NULL, 0);
}

int Shutdown(void){
    return CallYFS(CODE_SHUTDOWN, NULL, NULL, 0);
}

/*
    char* test1 = "/standard/directory/path/name";
    char* test2 = "/this/is/a/path/to/dir/";
    char* test3 = "/";
    char* test4 = NULL;
    char* test5 = "////this/has///a/bunch//////of//slashes////";
    char* test6 = "noslashes";
    char* test7 = "finally, this is a dot ending/";

    char* result1 = (char*)malloc(50);
    char* result2 = (char*)malloc(50);
    char* result3 = (char*)malloc(50);
    char* result4 = (char*)malloc(50);
    char* result5 = (char*)malloc(50);
    char* result6 = (char*)malloc(50);
    char* result7 = (char*)malloc(50);

    format_pathname(test1, result1);
    format_pathname(test2, result2);
    format_pathname(test3, result3);
    //format_pathname(test4, result4);
    format_pathname(test5, result5);
    format_pathname(test6, result6);
    format_pathname(test7, result7);
    
    printf("%s\n", result1);
    printf("%s\n", result2);
    printf("%s\n", result3);
    //printf("%s\n", result4);
    printf("%s\n", result5);
    printf("%s\n", result6);
    printf("%s\n", result7);
    */
    
/*
void print_bits_short(uint8_t n){
    int i;
    for(i = 0; i < 8; i++){
	if(n & 128)
	    printf("1");
	else
	    printf("0");
	n <<=1;
    }
}


void print_bits_long(uint64_t n){
    int i;
    for(i = 0; i < 64; i++){
	if(n & 9223372036854775808)
	    printf("1");
	else
	    printf("0");
	n <<=1;
	if(i % 8 == 7)
	    printf(" ");
    }
}
*/
