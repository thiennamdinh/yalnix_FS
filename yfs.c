#include "yfs.h"

struct dir_entry create_entry(short inum, char* filename){
    
    struct dir_entry entry;
    entry.inum = inum;

    memset(&entry.name, '\0', DIRNAMELEN);
    int namelen = strlen(filename);
    if(namelen > DIRNAMELEN)
	namelen = DIRNAMELEN;
    
    memcpy(&entry.name, filename, namelen);
    return entry;
}


int check_dir(int dir_inum, char* filename){

    struct inode_info* info = read_inode_from_disk(dir_inum);
    if(info->inode_number == ERROR){
	fprintf(stderr, "ERROR: could not read directory\n");
	return ERROR;
    }

    struct dir_entry entry;
    int num_entries = info->inode_val->size / sizeof(struct dir_entry);
    
    int i;
    for(i = 0; i < num_entries; i++){
	if(FSRead((void*)&entry, sizeof(entry), dir_inum, i * sizeof(struct dir_entry)) == ERROR){
	    return ERROR;
	}
 	if(strncmp(filename, entry.name, DIRNAMELEN) == 0){
	    return entry.inum;
	}
	
    }
    return 0;
}

int remove_from_dir(int dir_inum, int file_inum){
    struct inode_info* info = read_inode_from_disk(dir_inum);
    if(info->inode_number == ERROR){
	fprintf(stderr, "ERROR: could not read directory\n");
	return ERROR;
    }

    struct dir_entry entry;
    int num_entries = info->inode_val->size / sizeof(struct dir_entry);
    int i;
    for(i = 2; i < num_entries; i++){
	if(FSRead((void*)&entry, sizeof(entry), dir_inum, i * sizeof(struct dir_entry)) == ERROR){
	    return ERROR;
	}
	if(file_inum == entry.inum){
	    char* null_str = "\0";
	    struct dir_entry blank = create_entry(0, null_str);
	    FSWrite((void*)&blank, sizeof(blank), dir_inum, i * sizeof(struct dir_entry));
	    return 0;
	}
    }
    return ERROR;
}
 
int convert_pathname_to_inode_number(char *pathname, int proc_inum) {
    if(pathname == NULL ) {
	return proc_inum;
    }
    int cur_inode;
    char node_name[DIRNAMELEN + 1];
    memset(node_name,'\0',DIRNAMELEN + 1);
    int sym_link_cnt = 0;

    //Means that it is an absolute pathname, so it starts with ROOTINODE.
    if(pathname[0] == '/') {
	cur_inode = ROOTINODE;
    }
    else{
	cur_inode = proc_inum;
    }
    
    //Kills all the initial slashes.
    while(*pathname == '/'){
	pathname++;
    }

    while(strlen(pathname) != 0) {
        int len_path = strlen(pathname);
	memset(node_name,'\0',DIRNAMELEN + 1);
    	
	while(len_path > 0 && *pathname == '/') {
	    pathname++;
	    len_path--;
	}
	int i = 0;
	while(len_path > 0 && *pathname != '/') {
	    node_name[i] = *pathname;
	    i++;
	    pathname++;
	    len_path--;
	}
	struct inode_info* n;
        int sub_inum = check_dir(cur_inode, node_name);
        if (sub_inum<=0) {
	    fprintf(stderr, "ERROR: failed to parse given path\n");
	    return ERROR;
	}
	n = read_inode_from_disk(sub_inum);
	//If the sub inode is a symbolic link.
	if(n->inode_val->type == INODE_SYMLINK) {
	    if(sym_link_cnt >= MAXSYMLINKS) {
		fprintf(stderr, "ERROR: too many symlinks in pathname\n");
		return ERROR;
	    }else{
		int data_size = n->inode_val->size;
		char* new_pathname = (char*)calloc(sizeof(char) * data_size + len_path + 1, 1);
		struct block_info* b = read_block_from_disk(n->inode_val->direct[0]);
		memcpy(new_pathname, b->data, data_size);

		// append the rest of the unprocessed path name
		if(len_path > 0){
		    strcat(new_pathname, "/");
		    strcat(new_pathname, pathname);
		}
		pathname = new_pathname;

		sym_link_cnt++;
		// if symlink points to absolute, then reset current node to root
		if(new_pathname[0] == '/'){
		    cur_inode = ROOTINODE;
		    continue;
		}
	    }
	}
	cur_inode = sub_inum;
    }
    return cur_inode;
}

void print_dir(int dir_inum){
    
    struct inode_info* info = read_inode_from_disk(dir_inum);
    if(info->inode_number == ERROR){
	fprintf(stderr, "ERROR: could not read directory\n");
    }

    struct dir_entry entry;
    int num_entries = info->inode_val->size / sizeof(struct dir_entry);
    
    printf("\n---- directory %d ----\n", dir_inum);

    int i;
    for(i = 0; i < num_entries; i++){
	FSRead((void*)&entry, sizeof(entry), dir_inum, i * sizeof(struct dir_entry));
       	printf("%d - %s\n", entry.inum, entry.name);
    }
    printf("----------------------\n\n");
}


void print_file_blocks(short inum){
    struct inode_info* info = read_inode_from_disk(inum);
    int num_blocks = info->inode_val->size / BLOCKSIZE;

    printf("---- Blocks allocated for file %d -----\n", inum);

    int i;
    for(i = 0; i < num_blocks && i < NUM_DIRECT; i++){
	printf("%d : %d\n", i, info->inode_val->direct[i]);
    }

    if(i >= num_blocks)
	return;

    struct block_info* indirect_info = read_block_from_disk(info->inode_val->indirect);
    int* indirect = (int*)indirect_info->data;
    for(; i < num_blocks; i++){
	printf("%d : %d\n", i, indirect[i-NUM_DIRECT]);	
    }
}

int get_free_block() {
    int i ;
    for ( i = 0; i < NUM_BLOCKS; ++i){
	if(free_blocks[i] == FREE) {
	    free_blocks[i] = TAKEN;
	    return i;
	}

    }
    // if no free blocks found, attempt to claim blocks from deleted files
    sync();
    init_free();
    for ( i = 0; i < NUM_BLOCKS; ++i){
	if(free_blocks[i] == FREE) {
	    return i;
	}
    }

    fprintf(stderr, "ERROR: No space left on file system\n");
    return ERROR;
}

// allocate space for the file_inode to hold up to "newsize" data
int grow_file(struct inode_info* info, int newsize){
    struct inode* file_inode = info->inode_val;
    info->dirty = 1;
    if(newsize < file_inode->size){
	return 0;
    }

    info->dirty = 1;
    // round filesize up to the next blocksize
    int current = ((file_inode->size + (BLOCKSIZE-1)) / BLOCKSIZE) * BLOCKSIZE;
    // fill up direct blocks first
    if(current < BLOCKSIZE * NUM_DIRECT){
        while(current < BLOCKSIZE * NUM_DIRECT && current < newsize){
	    // assign a new block in direct
	    int free_block = get_free_block();
	    if(free_block == ERROR) {
		return ERROR;
	    }

	    struct block_info* info = read_block_from_disk(free_block);
	    info->dirty = 1;
	    memset(info->data, '\0', BLOCKSIZE);

	    file_inode->direct[current / BLOCKSIZE] = free_block;
	    current += BLOCKSIZE;
        }
    }
    
    // If this is the first time growing into indirect size then allocate indirect block
    if(current < newsize && current == BLOCKSIZE * NUM_DIRECT){
        int new_indirect = get_free_block();
	if(new_indirect == ERROR)
	    return ERROR;
	file_inode->indirect = new_indirect;
    }

    // if direct blocks not enough, then access indirect blocks
    if(current < newsize){
	int big_block_num = file_inode->indirect;
	struct block_info * block_indirect = read_block_from_disk(big_block_num);
	block_indirect->dirty = 1;
	int * int_array = (int*)(block_indirect->data);
	
        while(current < BLOCKSIZE * (NUM_DIRECT + BLOCKSIZE / sizeof(int)) && current < newsize ) {
	    int free_block = get_free_block();
	    if(free_block == ERROR) {
		return ERROR;
	    }
	    int_array[current / BLOCKSIZE - NUM_DIRECT] = free_block;
	    current += BLOCKSIZE;
	}
    }
    file_inode->size = newsize;
    return 0;
}

// returns a pointer to the data starting at "position" in the file described by "file_inode"
char* get_data_at_position(struct inode_info* info, int position, int set_dirty){
    struct inode* file_inode = info->inode_val;
    if(position > file_inode->size){
	fprintf(stderr, "ERROR: trying to read past size of file");
	return NULL;
    }

    int file_block_num = position / BLOCKSIZE;

    // if position is within direct blocks
    if(file_block_num < NUM_DIRECT){
	struct block_info* direct_info = read_block_from_disk(file_inode->direct[file_block_num]);
	if(direct_info->dirty != 1 && set_dirty == 1)
	    direct_info->dirty = set_dirty;
        return direct_info->data + position % BLOCKSIZE;
    }

    // if position is within indirect blocks
    struct block_info* indirect_info = read_block_from_disk(file_inode->indirect);
    int target_num = ((int*)(indirect_info->data))[file_block_num - NUM_DIRECT];

    struct block_info* target_info = read_block_from_disk(target_num);
    target_info->dirty = set_dirty;
    return ((char*)(target_info->data)) + position % BLOCKSIZE;
}

int add_directory_entry(short dir_inum, struct dir_entry new_entry){
    struct inode_info* dir_info = read_inode_from_disk(dir_inum);
    dir_info->dirty = 1;

    if(dir_info->inode_number == -1 || dir_info->inode_val->type != INODE_DIRECTORY){
	fprintf(stderr, "ERROR: not a valid directory inode number\n");
    }

    int dir_size = dir_info->inode_val->size;
    int position = 0;
    struct dir_entry old_entry;
    
    // look for a blank entry in the middle of the directory first and overwrite with new entry
    while(position < dir_size){
	FSRead(&old_entry, sizeof(old_entry), dir_inum, position);
	if(old_entry.inum == 0){
	    int success = FSWrite(&new_entry, sizeof(new_entry), dir_inum, position);
	    if(success != ERROR){
		struct inode_info* info = read_inode_from_disk(new_entry.inum);
		info->inode_val->nlink++;
		info->dirty = 1;
		return success;
	    }
	    else{
		return ERROR;
	    }
	}
	position += sizeof(old_entry);
    }
    
    // if none available, write new entry at the end of the file
    int success = FSWrite(&new_entry, sizeof(new_entry), dir_inum, position);

    if(success != ERROR){
	struct inode_info* info = read_inode_from_disk(new_entry.inum);
	info->inode_val->nlink++;
	info->dirty = 1;
	return success;
    }
    else{
	return ERROR;
    }
}

int get_parent_inum(char* pathname, short current_dir){
    // parse backwards and look for last '/'
    int i;
    for(i = strlen(pathname) - 1; i >= 0; i--){
	if(pathname[i] == '/'){
	    break;
	}
    }
    int parent_path_len = i + 1;
    
    // attempt to get inode of parent directory
    char* parent_path = (char*)malloc(parent_path_len + 1);
    memcpy(parent_path, pathname, parent_path_len);
    parent_path[parent_path_len] = '\0';

    // strip the trailing '/' unless the parent directory is root
    if(!(parent_path_len == 1 && parent_path[0] == '/')){
	parent_path[parent_path_len-1] = '\0';
    }

    short parent_inum = convert_pathname_to_inode_number(parent_path, current_dir);
    free(parent_path);

    if(parent_inum == ERROR){
	fprintf(stderr, "ERROR: failed to obtain path to parent directory\n");
	return ERROR;
    }
    return parent_inum;
}

// takes a pathname and returns a pointer to the name of the last file in the path
// note that the pointer points to a section of the original parameter
char* get_filename(char* pathname){
    int i;
    for(i = strlen(pathname) - 1; i >= 0; i--){
	if(pathname[i] == '/'){
	    return pathname + i + 1;
	}
    }
    return pathname;
}

// creates a new file under the given parent directory
int create_file(char* filename, short parent_inum, int type){
    short file_inum = check_dir(parent_inum, filename);

    if(file_inum == ERROR){
	return ERROR;
    }
    else if(file_inum != 0){
	fprintf(stderr, "ERROR: file already exists\n");
	return ERROR;
    }

    // allocate new inode for file
    short i;
    for(i = 0; i < NUM_INODES; i++){
	if(free_inodes[i] == FREE){
	    file_inum = i;
	    break;
	}
    }
    if(file_inum == NUM_INODES){
	fprintf(stderr, "ERROR: no more inodes left for new file\n");
	return ERROR;
    }

    free_inodes[i] = TAKEN;
    
    struct inode_info* file_info = read_inode_from_disk(file_inum);
    struct inode* file_inode = file_info->inode_val;
    file_inode->type = type;
    file_inode->nlink = 0;
    file_inode->reuse++;
    file_inode->size = 0;
    file_info->dirty = 1;

    // create and populate new directory entry
    struct dir_entry entry = create_entry(file_inum, filename);

    if(add_directory_entry(parent_inum, entry) == ERROR){
	// undo prior process to create inode for the file
	free_inodes[file_inum] = FREE;
	file_inode->type = INODE_FREE;
	file_inode->nlink = 0;
	file_inode->reuse--;
	
	fprintf(stderr, "ERROR: failed to add file to directory\n");
	return ERROR;
    }
    return file_inum;
}

void init_free(){
    
    struct inode_info* i_info = read_inode_from_disk(0);
    struct fs_header* header = (struct fs_header*)(i_info->inode_val);

    NUM_INODES = header->num_inodes;
    NUM_BLOCKS = header->num_blocks;

    free_inodes = (short*)malloc(NUM_INODES * sizeof(short));
    free_blocks = (short*)malloc(NUM_BLOCKS * sizeof(short));
     
    int i;
    for (i = 0; i < NUM_BLOCKS; ++i){
	free_blocks[i] = FREE;
    }
    for (i = 0; i < NUM_INODES; ++i){
	free_inodes[i] = FREE;
    }

    free_inodes[0] = TAKEN;  // fs_header inode is taken   
    free_inodes[1] = TAKEN;  // root inode taken   
    free_blocks[0] = TAKEN;  // boot block is taken
    // inode blocks are taken
    for(i = 1; i < 1 + ((NUM_INODES + 1) * INODESIZE) / BLOCKSIZE; i++){
	free_blocks[i] = TAKEN;
    }

    // loop through all inodes
    for(i = 1; i < NUM_INODES + 1; i++){
	struct inode* current_inode = read_inode_from_disk(i)->inode_val;
    
	if(current_inode->type != INODE_FREE){
	    free_inodes[i] = TAKEN;
	    int j = 0;
	    // loop through direct blocks
	    while(j < NUM_DIRECT && j * BLOCKSIZE < current_inode->size){
		free_blocks[current_inode->direct[j]] = TAKEN;
		j++;		    
	    }

	    // if file still has more blocks, explore indirect block as well
	    if(j * BLOCKSIZE < current_inode->size){
		int* indirect_block = (int*)(read_block_from_disk(current_inode->indirect)->data);
		free_blocks[current_inode->indirect] = TAKEN;
		int last_block = (current_inode->size + (BLOCKSIZE-1)) / BLOCKSIZE;

		while(j < last_block){
		    free_blocks[indirect_block[j - NUM_DIRECT]] = TAKEN;
		    j++;
		}

	    }
	}
    }
}

int FSOpen(char *pathname, short current_dir){
    return convert_pathname_to_inode_number(pathname, current_dir);
}

int FSCreate(char *pathname, short current_dir){
    short parent_inum = get_parent_inum(pathname, current_dir);
    char* filename = get_filename(pathname);
    return create_file(filename, parent_inum, INODE_REGULAR);
}

int FSRead(void *buf, int size, short inode, int position){
    struct inode_info* info = read_inode_from_disk(inode);
    if(info->inode_number == -1){
	fprintf(stderr, "ERROR: not a valid inode number\n");
	return ERROR;
    }
    struct inode* file_inode = info->inode_val;

    int offset = 0;
    // keep reading while buf is not full and we have not reached the end of the file
    while(offset < size && position + offset < file_inode->size){
        char* data = get_data_at_position(info, position + offset, 0);
	// readable size is min of space left in the block, buffer, and file
	int readable_size = readable_size = BLOCKSIZE - (position + offset) % BLOCKSIZE;
	if(size - offset < readable_size)
	    readable_size = size - offset;
	if(file_inode->size - (position + offset) < readable_size)
	    readable_size = file_inode->size - (position + offset);
	memcpy(buf + offset, data, readable_size);
	offset += readable_size;
    }
    
    return offset;
}

int FSWrite(void *buf, int size, short inum, int position){
    struct inode_info* info = read_inode_from_disk(inum);
    info->dirty = 1;
    if(info->inode_number == -1){
	fprintf(stderr, "ERROR: not a valid inode number\n");
	return ERROR;
    }
    // if writing past the size of the current file then we need to expand it first
    if(grow_file(info, position + size) == ERROR){
	fprintf(stderr, "ERROR: failed to grow file in write operation\n");
	return ERROR;
    }

    int offset = 0;
    while(offset < size && position + offset < info->inode_val->size){
	char * data = get_data_at_position(info, position + offset, 1);

	// writeable size is min of blocksize and space left in the buf
	int writeable_size = BLOCKSIZE - (position + offset) % BLOCKSIZE;
	if(size - offset < writeable_size)
	    writeable_size = size - offset;

	memcpy(data, buf + offset, writeable_size);
	offset += writeable_size;
    }
    return offset;
}

// This just returns the size of a given file... it's only necessary for a particular case of
// seek in the iolib
int FSSeek(short inode){
    struct inode_info* info = read_inode_from_disk(inode);
    if(info->inode_number == -1){
	fprintf(stderr, "ERROR: not a valid inode number\n");
	return ERROR;
    }
    return info->inode_val->size;
}

int FSLink(char *oldname, char *newname, short current_dir){

    short old_inum = convert_pathname_to_inode_number(oldname, current_dir);

    struct inode_info* old_info = read_inode_from_disk(old_inum);
    if(old_info->inode_number == -1 || old_info->inode_val->type == INODE_DIRECTORY){
	fprintf(stderr, "ERROR: cannot create link to given file\n");
	return ERROR;
    }
    
    short parent_inum = get_parent_inum(newname, current_dir);
    char* new_filename = get_filename(newname);

    struct dir_entry entry = create_entry(old_inum, new_filename);
    add_directory_entry(parent_inum, entry);

    old_info->inode_val->nlink++;
    old_info->dirty = 1;
    return 0;
}

int FSUnlink(char *pathname, short current_dir){

    short inum = convert_pathname_to_inode_number(pathname, current_dir);
    struct inode_info* info = read_inode_from_disk(inum);
    if(info->inode_number == -1 || info->inode_val->type == INODE_DIRECTORY){
	fprintf(stderr, "ERROR: cannot read link to given file\n");
	return ERROR;
    }

    short parent_inum = get_parent_inum(pathname, current_dir);
    remove_from_dir(parent_inum, inum);

    info->inode_val->nlink--;
    info->dirty = 1;
    
    return 0;
}

int FSSymLink(char *oldname, char *newname, short current_dir){
    short parent_inum = get_parent_inum(newname, current_dir);
    char* filename = get_filename(newname);
    short inum = create_file(filename, parent_inum, INODE_SYMLINK);
    if(inum == ERROR)
	return ERROR;

    int result = FSWrite((void*)oldname, strlen(oldname), inum, 0);
    return result;
}

int FSReadLink(char *pathname, char *buf, int len, short current_dir){
    short parent_inum = get_parent_inum(pathname, current_dir);
    char* filename = get_filename(pathname);
    short inum = check_dir(parent_inum, filename);
    return FSRead(buf, len, inum, 0);
}

int FSMkDir(char *pathname, short current_dir){
    short parent_inum = get_parent_inum(pathname, current_dir);
    char* filename = get_filename(pathname);
    short inum = create_file(filename, parent_inum, INODE_DIRECTORY);
    if(inum == ERROR)
	return ERROR;

    char* strdot = ".";
    char* strdotdot = "..";
    
    struct dir_entry dot = create_entry(inum, strdot);
    struct dir_entry dotdot = create_entry(parent_inum, strdotdot);

    add_directory_entry(inum, dot);
    add_directory_entry(inum, dotdot);
 
    return 0;
}

int FSRmDir(char *pathname, short current_dir){

    if(strcmp(pathname, "/") * strcmp(pathname, ".") * strcmp(pathname, "..") == 0){
	fprintf(stderr, "ERROR: attempting to remove protected path name\n");
	return ERROR;
    }
    
    short inum = convert_pathname_to_inode_number(pathname, current_dir);
    struct inode_info* info = read_inode_from_disk(inum);
    
    if(info->inode_number == -1 || info->inode_val->type != INODE_DIRECTORY){
	fprintf(stderr, "ERROR: given pathname is not a valid directory\n");
	return ERROR;
    }

    // check to make sure that any entries in directory after . and .. are empty
    int i;
    for(i = 2; i < info->inode_val->size / sizeof(struct dir_entry); i++){
	struct dir_entry entry;
	FSRead((void*)&entry, sizeof(entry), inum, i * sizeof(entry));
	if(entry.inum != 0){
	    fprintf(stderr, "ERROR: cannot remove non-empty directory\n");
	    return ERROR;
	}
    }

    short parent_inum = get_parent_inum(pathname, current_dir);    
    remove_from_dir(parent_inum, inum);

    info->inode_val->type = INODE_FREE;    
    free_inodes[inum] = FREE;
    info->dirty = 1;

    return 0;
}

int FSChDir(char *pathname, short current_dir){
    short inum = convert_pathname_to_inode_number(pathname, current_dir);
    if(read_inode_from_disk(inum)->inode_val->type != INODE_DIRECTORY){
	fprintf(stderr, "ERROR: pathname does not lead to directory\n");
	return ERROR;
    }
    return inum;
}

int FSStat(char *pathname, struct Stat* statbuf, short current_dir){
    short inum = convert_pathname_to_inode_number(pathname, current_dir);
    struct inode_info* info = read_inode_from_disk(inum);
    if(info->inode_number == -1){
	fprintf(stderr, "ERROR: pathname not valid\n");
	return ERROR;
    }
    statbuf->inum = info->inode_number;
    statbuf->type = info->inode_val->type;
    statbuf->size = info->inode_val->size;
    statbuf->nlink = info->inode_val->nlink;

    return 0;
}

int FSSync(void){
    return sync();
}

int FSShutdown(void){
    sync(); 
    return 0;
}

int Redirect_Call(char* msg, int pid){
#pragma GCC diagnostic push /*ignore unavoidable gcc warning caused by unconventional casting */
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
    
    char* current = msg;
    uint8_t code = (uint8_t)(msg[0]);
    int result;
        
    switch(code){
	case CODE_OPEN:{
	    char* upathname; int upathname_size; short ucurrent_dir;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ucurrent_dir, current += sizeof(upathname_size), sizeof(ucurrent_dir));

	    char* pathname = (char*)calloc(upathname_size + 1, sizeof(char));
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSOpen(pathname, ucurrent_dir);
	    free(pathname);
	    break;
	}
	case CODE_CREATE:{
	    char* upathname; int upathname_size; short ucurrent_dir;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ucurrent_dir, current += sizeof(upathname_size), sizeof(ucurrent_dir));

	    char* pathname = (char*)calloc(upathname_size + 1, sizeof(char));
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSCreate(pathname, ucurrent_dir);
	    free(pathname);
	    break;
	}
	case CODE_READ:{
	    char* ubuf; int usize; short uinode; int uposition;
	    memcpy(&ubuf, current += sizeof(code), sizeof(ubuf));
	    memcpy(&usize, current += sizeof(ubuf), sizeof(usize));
	    memcpy(&uinode, current += sizeof(usize), sizeof(uinode));
	    memcpy(&uposition, current += sizeof(uinode), sizeof(uposition));
 
	    char* buf = calloc(usize + 1, sizeof(char));
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

	    char* buf = calloc(usize + 1, sizeof(char));
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
	    short ucurrent_dir;
	    memcpy(&uoldname, current += sizeof(code), sizeof(uoldname));
	    memcpy(&uoldname_size, current += sizeof(uoldname), sizeof(uoldname_size));
	    memcpy(&unewname, current += sizeof(unewname_size), sizeof(unewname));
	    memcpy(&unewname_size, current += sizeof(unewname), sizeof(unewname_size));
	    memcpy(&ucurrent_dir, current += sizeof(unewname_size), sizeof(ucurrent_dir));

	    char* oldname = calloc(uoldname_size + 1, sizeof(char));
	    char* newname = calloc(unewname_size + 1, sizeof(char));
	    CopyFrom(pid, oldname, uoldname, uoldname_size + 1);
	    CopyFrom(pid, newname, unewname, unewname_size + 1);
	    result = FSLink(oldname, newname, ucurrent_dir);
	    break;
	}
	case CODE_UNLINK:{
	    char* upathname; int upathname_size; short ucurrent_dir;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ucurrent_dir, current += sizeof(upathname_size), sizeof(ucurrent_dir));

	    char* pathname = (char*)calloc(upathname_size + 1, sizeof(char));
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSUnlink(pathname, ucurrent_dir);
	    free(pathname);
	    break;
	}
	case CODE_SYMLINK:{
	    char* uoldname; int uoldname_size; char* unewname; int unewname_size;
	    short ucurrent_dir;
	    memcpy(&uoldname, current += sizeof(code), sizeof(uoldname));
	    memcpy(&uoldname_size, current += sizeof(uoldname), sizeof(uoldname_size));
	    memcpy(&unewname, current += sizeof(unewname_size), sizeof(unewname));
	    memcpy(&unewname_size, current += sizeof(unewname), sizeof(unewname_size));
	    memcpy(&ucurrent_dir, current += sizeof(unewname_size), sizeof(ucurrent_dir));

	    char* oldname = calloc(uoldname_size + 1, sizeof(char));
	    char* newname = calloc(unewname_size + 1, sizeof(char));
	    CopyFrom(pid, oldname, uoldname, uoldname_size + 1);
	    CopyFrom(pid, newname, unewname, unewname_size + 1);
	    result = FSSymLink(oldname, newname, ucurrent_dir);
	    break;
	}
	case CODE_READLINK:{
	    char* upathname; int upathname_size; char* ubuf; int ulen; short ucurrent_dir;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ubuf, current += sizeof(upathname_size), sizeof(ubuf));
	    memcpy(&ulen, current += sizeof(ubuf), sizeof(ulen));
	    memcpy(&ucurrent_dir, current += sizeof(ulen), sizeof(ucurrent_dir));

	    char* pathname = calloc(upathname_size + 1, sizeof(char));
	    char* buf = calloc(ulen + 1, sizeof(char));
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSReadLink(pathname, buf, ulen, ucurrent_dir);
	    CopyTo(pid, ubuf, buf, ulen + 1);
	    free(pathname);
	    free(buf);
	    break;
	}
	case CODE_MKDIR:{
	    char* upathname; int upathname_size; short ucurrent_dir;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ucurrent_dir, current += sizeof(upathname_size), sizeof(ucurrent_dir));

	    char* pathname = (char*)calloc(upathname_size + 1, sizeof(char));
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSMkDir(pathname, ucurrent_dir);
	    free(pathname);
	    break;
	}
	case CODE_RMDIR:{
	    char* upathname; int upathname_size; short ucurrent_dir;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ucurrent_dir, current += sizeof(upathname_size), sizeof(ucurrent_dir));

	    char* pathname = (char*)calloc(upathname_size + 1, sizeof(char));
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSRmDir(pathname, ucurrent_dir);
	    free(pathname);
	    break;
	}
	case CODE_CHDIR:{
	    char* upathname; int upathname_size; short ucurrent_dir;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ucurrent_dir, current += sizeof(upathname_size), sizeof(ucurrent_dir));

	    char* pathname = (char*)calloc(upathname_size + 1, sizeof(char));
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSChDir(pathname, ucurrent_dir);
	    free(pathname);
	    break;
	}
	case CODE_STAT:{
	    char* upathname; int upathname_size; struct Stat* ustatbuf; short ucurrent_dir;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ustatbuf, current += sizeof(upathname_size), sizeof(ustatbuf));
	    memcpy(&ucurrent_dir, current += sizeof(ustatbuf), sizeof(ucurrent_dir));

	    char* pathname = (char*)calloc(upathname_size + 1, sizeof(char));
	    struct Stat* statbuf = (struct Stat*)calloc(1, sizeof(struct Stat));
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSStat(pathname, statbuf, ucurrent_dir);
	    CopyTo(pid, ustatbuf, statbuf, sizeof(struct Stat));
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
	    if(result == 0){
		// clean msg
		int i;
		for(i = 0; i < MESSAGE_SIZE; i++){
		    msg[i] = '\0';
		}
		// copy in result and reply
		memcpy(msg, &result, sizeof(result));
		Reply(msg, pid);      
		printf("\nShutdown request successful. Terminating Yalnix File System.\n");
		Exit(0);
	    }
	    break;
	}
	default:{
	    result = ERROR;
	}

    }
    return result;
#pragma GCC diagnostic pop /*pop -Wint-to-pointer-cast ignore warning*/
}

int main(int argc, char** argv){
    
    init();
    init_free();    
    Register(FILE_SERVER);
    printf("Initialized File System\n");

    int pid = Fork();
    if(pid == 0){
	Exec(argv[1], argv + 1);
	printf("No init file provided. Halting machine new\n");
	Halt();
    }
    
    // stand by and simply route messages from here on out
    while(1){
	char msg[MESSAGE_SIZE];
	int pid = Receive(msg);
	if(pid == -1){
	    fprintf(stderr, "Receive() returned error\n");
	    continue;
	}
	if(pid == 0){
	    Pause();
	    fprintf(stderr, "Recieve() returned 0 to avoid deadlock\n");
	    continue;
	}
	
	int result = Redirect_Call(msg, pid);

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
}
