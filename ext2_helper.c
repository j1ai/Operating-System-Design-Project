
#include "ext2.h"
#include "ext2_helper.h"

// helper for string

unsigned char *disk;
char * get_parent_path(const char *path){
    return dirname(strdup(path));
}

char *get_base_name(const char *path){
    return basename(strdup(path));
}


/*
 *  Return the mmaped disk from the 
 */

unsigned char *create_disk(char *image){
    
    int fd = open(image, O_RDWR);
     
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    
    return disk;
}

struct ext2_super_block *get_sb(){
    return (struct ext2_super_block *)(disk + 1024);
}

/*
 *  Return the Block Group Descriptor of this disk
 */

struct ext2_group_desc *get_bgd(){
    return (struct ext2_group_desc *)(disk + 2048);
}

/*
 * Return the Block Bitmap from the current block group descriptor 
 */
unsigned char *get_block_bitmap(){
    return (disk + EXT2_BLOCK_SIZE*(get_bgd())->bg_block_bitmap);
}
/*
 * Return the Inode Bitmap from the current block group descriptor 
 */
unsigned char *get_inode_bitmap(){
    return (disk + EXT2_BLOCK_SIZE*(get_bgd())->bg_inode_bitmap);
}
/*
 * Return the Block Bitmap from the current block group descriptor 
 */
unsigned char *get_inode_table(){
    return (disk + EXT2_BLOCK_SIZE*(get_bgd())->bg_inode_table);
}

/*
 * Return the Inode for specified index (start from 1)
 */
struct ext2_inode *get_inode(int ind){
    return (struct ext2_inode *)(get_inode_table()+ (sizeof(struct ext2_inode) * (ind- 1)));
}

/*
 * Return Root Inode (index 2)
 */
struct ext2_inode *get_root_inode(){
    return get_inode(2);
}

/*
 * Return the Inode for specified index (start from 1)
 */
unsigned char *get_block(int ind){
    return disk + (ind)*EXT2_BLOCK_SIZE;
}



struct ext2_dir_entry *find_sub_entry(struct ext2_inode *c_inode, char *dir_name){
        if (dir_name == NULL || strcmp(dir_name, "\0")==0){
            return NULL;
        }
        struct ext2_dir_entry *entry;
        //printf("%d \n",c_inode->i_block[0]);// first block is an array
        int i;
        char *name;
        int count_len;
        for (i = 0; i<12; i++) {
            //If the inodes of that block is in use
            if (c_inode->i_block[i] == 0) {
                break;
            }

            // Pointed to the first entry stored in this block
            count_len = 0;
            while (count_len < EXT2_BLOCK_SIZE) {
                entry = (void *)get_block(c_inode->i_block[i])+count_len;
                // If this directory file is in use
                if (entry->inode != 0) {
                    name = entry->name;
                    if (entry->name_len == strlen(dir_name) && (strcmp(dir_name, name) == 0)) {
                        if(entry->file_type == EXT2_FT_DIR){ // if this is a directory get into it.
                            
                            struct ext2_inode *result_inode = get_inode(entry->inode);
                            entry = (struct ext2_dir_entry *)get_block(result_inode->i_block[0]);
                        }
                        return entry;
                    }
                }

                count_len += entry->rec_len;
            }
        }
        // we dont need to deal with block 13 14 15.
    return  NULL;
}


struct ext2_dir_entry *find_entry(const char *path){
    if (path == NULL){
        return NULL;
    }
    if (strcmp(path,"/") == 0){
        return (struct ext2_dir_entry *)get_block(get_root_inode()->i_block[0]);
    }
    char *target_path = strdup(path);
    struct ext2_dir_entry *result_entry = NULL;
    struct ext2_inode *cur_node = get_root_inode();
    const char slash[2] = "/";
    char *cur_dir_name = strtok(target_path ,slash); // use to store the current directory we are checking.
    while(cur_dir_name != NULL){
        result_entry = find_sub_entry(cur_node, cur_dir_name);
        if (result_entry == NULL){
            return result_entry;
        }
        cur_node = get_inode(result_entry->inode);
        cur_dir_name = strtok(NULL,slash);
    }

    if(result_entry && result_entry->file_type == EXT2_FT_DIR){ // if this is a directory get into it.
        struct ext2_inode *result_inode= get_inode(result_entry->inode);
        result_entry = (struct ext2_dir_entry *)get_block(result_inode->i_block[0]);
    }
    return result_entry;
}


unsigned short fourb_alignment (unsigned short rec_len){
    while (rec_len %4 != 0){
        rec_len ++;
    }
    return rec_len;
}





unsigned int get_new_block_no() {
    unsigned char *block_bits = get_block_bitmap();
    int byte;
    int bit;
    //Not enough memory to assign new inode
    if (get_sb()->s_free_blocks_count == 0) {
        fprintf(stderr, "There is not eonough new memeory to allocate new blcok \n");
        exit(ENOMEM);
    }
    for (byte = 0; byte < (get_sb()->s_blocks_count) / 8; byte++) {
        for (bit = 0; bit < 8; bit++) {
            int in_use = block_bits[byte]>>bit & 1;
            // Inode is not in used
            if (in_use == 0) {
                //Set this inode to be inused
                block_bits[byte] |= 1<<bit;
                //return this i_node
                unsigned int block_no = (unsigned int)((byte * 8) + bit) + 1;
                get_bgd()->bg_free_blocks_count--;
                get_sb()->s_free_blocks_count--;
                return block_no;
            }
        }
    }
    return 0;
}

/*
 * Return the inserted entry inserted to cur_entry(which is a directory).
 * only rec_len is setted.
 *
 */
struct ext2_dir_entry *create_entry(struct ext2_dir_entry *parent_entry, int size_needed){
    struct ext2_dir_entry *temp_entry;
    unsigned int *cur_block = get_inode(parent_entry->inode)->i_block;
    int i;
    int count_len;
    struct ext2_dir_entry *new_entry;
    
    //The first 12 blocks are direct blocks
    for (i = 0; i<12 ; i++){
    //If the inodes are in used and it is not reserved
        if (cur_block[i] != 0 ){
            // Set the blocks number where cur_block[i] is pointed to
            count_len = 0;

            while (count_len < EXT2_BLOCK_SIZE){
                temp_entry = (void *)get_block(cur_block[i])+count_len;
                unsigned short actual_size = sizeof(struct ext2_dir_entry)+temp_entry->name_len;
                actual_size = fourb_alignment(actual_size);
                if ((temp_entry->rec_len - actual_size) > size_needed){

                    unsigned short new_reclen = temp_entry->rec_len - actual_size;
                    // there is a gap.
                    temp_entry->rec_len = actual_size;
                    new_entry = (void*)get_block(cur_block[i])+count_len+temp_entry->rec_len;// just here!
                    new_entry->rec_len = new_reclen;

                    return new_entry;
                }
                count_len += temp_entry->rec_len;
            }
        }else{
            cur_block[i] = get_new_block_no();
            new_entry = (struct ext2_dir_entry*)(get_block(cur_block[i]));
            new_entry->rec_len = EXT2_BLOCK_SIZE;
            return new_entry;
        }
    }
    return NULL;
}


/*
 * Allocate a new inode return the inode number.
 */
unsigned int get_new_inode_no (){
    unsigned char *inode_bits = get_inode_bitmap();
    struct ext2_super_block *sb = get_sb();
    int byte;
    int bit;
    //Not enough memory to assign new inode
    if (sb->s_free_inodes_count == 0){
        fprintf(stderr,"There is not eonough new memeory to allocate new inode \n");
        exit(ENOMEM);
    }
    for (byte = 0; byte<(sb->s_inodes_count)/8;byte++){
        for(bit = 0;bit<8;bit++){
            int in_use = inode_bits[byte]>>bit & 1;
            // Inode is not in used
            if (in_use == 0){
                //Set this inode to be inused
                inode_bits[byte] |= 1<<bit;
                get_bgd()->bg_free_inodes_count--;
                get_sb()->s_free_inodes_count--;
                //return this i_node
                unsigned int node_no = (unsigned int) ((byte*8) + bit)+1;
                struct ext2_inode *new_inode = get_inode(node_no); // Inode offset
                new_inode->i_links_count = 1;

                return node_no;
            }
        }
    }
    return 0;
}
//
int get_blocks_number(struct ext2_inode *inode){
    return (inode->i_blocks * 512) / EXT2_BLOCK_SIZE;
}

void update_iblocks(struct ext2_inode *inode){
    int i;
    unsigned int count = 0;
    for(i=0;i<12; i++ ){
        if (inode->i_block[i] != 0){
            count+= EXT2_BLOCK_SIZE;
        }
    }
    inode->i_blocks = count;
}


