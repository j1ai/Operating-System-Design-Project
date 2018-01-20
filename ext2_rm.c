#include "ext2.h"
#include "ext2_helper.h"

unsigned char *disk;

int delete_entry(struct ext2_dir_entry *previous, struct ext2_dir_entry *target){
    struct ext2_inode *target_inode = get_inode(target->inode);
    if(previous == NULL){
        target->inode = 0; // If this is first one then just set inode to zero
    }else{
        previous->rec_len = target->rec_len + previous->rec_len; // else just set rec len of previous one.
    }
    int blocks_needed = get_blocks_number(target_inode);
    int i;
    for(i=0;i < blocks_needed;i++){
        get_block_bitmap()[target_inode->i_block[i]] = 0;
        get_bgd()->bg_free_blocks_count ++;
    }
    get_bgd()->bg_free_inodes_count ++;
    return 0;
}


int search_and_delete(struct ext2_dir_entry *parent, struct ext2_dir_entry *target){
    int block_used = get_inode(parent->inode)->i_blocks*512/EXT2_BLOCK_SIZE;
    unsigned int *blocks = get_inode(parent->inode)->i_block;
    int i;
    struct ext2_dir_entry *entry;
    struct ext2_dir_entry *previous = NULL;
    int count;
    for(i=0;i<block_used;i++){
        count = 0;
        while (count < EXT2_BLOCK_SIZE) {
            entry = (struct ext2_dir_entry *)(get_block(blocks[i])+count);
            if (entry == target){
                delete_entry(previous,entry);
            }
            previous = entry;
            count += entry->rec_len;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <Image> <absolute path on this image> \n", argv[0]);
        exit(1);
    }

    char *image = argv[1];
    char *remove_path = argv[2];
    disk = create_disk(image);
    char *parent_path = get_parent_path(remove_path);

    struct ext2_dir_entry *target_entry = find_entry(remove_path);
    if(target_entry == NULL){
        perror("Does not exist");
        return EEXIST;
    }

    if(target_entry->file_type & EXT2_FT_DIR ){
        perror("Can not delete Directory");
        return ENOENT;
    }
    // Processing deletion
    struct ext2_dir_entry *target_parent = find_entry(parent_path);
    search_and_delete(target_parent,target_entry);
    return 0;
}
