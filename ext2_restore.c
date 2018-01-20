#include "ext2.h"
#include "ext2_helper.h"

unsigned char *disk;
int try_to_restore(struct ext2_dir_entry *target){
    if(get_inode_bitmap()[target->inode] == 1){
        perror("File has been lost");
        return  ENOENT;
    }

    struct ext2_inode *tgt_inode = get_inode(target->inode);
    if(tgt_inode->i_mode & EXT2_S_IFDIR){
        perror("Can not restore a directory");
        return  ENOENT;
    }

    int i;
    for(i=0;i< get_blocks_number(tgt_inode);i++){
        if(tgt_inode->i_block[i]!=0){
            if(get_block_bitmap()[tgt_inode->i_block[i]]){
                perror("File has been lost");
                return  ENOENT;
            }
        }
    }

    get_inode_bitmap()[target->inode] |= 1;
    get_sb()->s_free_inodes_count--;
    for(i=0;i< get_blocks_number(tgt_inode);i++){
        if(tgt_inode->i_block[i]!=0){
            get_block_bitmap()[tgt_inode->i_block[i]] |= 1;
            get_sb()->s_free_inodes_count--;
        }
    }
    return 0;
}

int restore_entry_by_name(struct ext2_dir_entry *parent, char* name){
    struct ext2_inode *parent_inode = get_inode(parent->inode);
    int block_used = get_blocks_number(parent_inode);
    unsigned int *blocks = parent_inode->i_block;
    int i;
    struct ext2_dir_entry *entry;
    struct ext2_dir_entry *target_entry; // use to restore
    int count;
    int success;
    unsigned short size_needed;
    for(i=0;i<block_used;i++){
        count = 0;
        while(count < EXT2_BLOCK_SIZE){
            entry = (struct ext2_dir_entry *)(get_block(blocks[i]) + count);
            size_needed = fourb_alignment(entry->name_len + sizeof(struct ext2_dir_entry));
            if(entry->rec_len > size_needed) {
                target_entry = entry + size_needed;
                if (strcmp(target_entry->name,name) == 0){
                    success = try_to_restore(target_entry);
                    if(success==0){
                        entry->rec_len = size_needed; // restore the reclen of the previous one.
                    }
                    return success;
                }

            }
            count += entry->rec_len;
        }

    }
    return 0;
}



int main(int argc, char **argv) {
    
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <image file name> <absolute path on this image> \n", argv[0]);
        exit(1);
    }

    char *image = argv[1];
    char *target_path = argv[2];
    disk = create_disk(image);
    char *remove_path_parent = get_parent_path(target_path);

    struct ext2_dir_entry *target_parent = find_entry(remove_path_parent);
    if(target_parent == NULL){
        perror("Parent does not exist");
        return EEXIST;
    }

    return restore_entry_by_name(target_parent,get_base_name(target_path));

}