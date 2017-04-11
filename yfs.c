#include <comp421/iolib.h>

block_wrap *block_head, *block_tail;

inode_wrap *inode_head, *inode_tail;

block_wrap* get_lru_block(int block_number) {

}

int put_lru_block(block_wrap *block) {

}

inode_wrap* get_lru_inode(int inode_number) {

}

int put_lru_inode(inode_wrap *inode) {

}


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
