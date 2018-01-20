#include "ext2.h"
#include "ext2_helper.h"


unsigned char *disk;

int fix_file_type(unsigned const short *imode,unsigned char *ftype){
    if (*imode == EXT2_S_IFLNK && *ftype != EXT2_FT_SYMLINK){
        *ftype = EXT2_FT_SYMLINK;
        return 1;
    }
    else if (*imode == EXT2_S_IFREG && *ftype != EXT2_FT_REG_FILE){
        *ftype = EXT2_FT_REG_FILE;
        return 1;
    }
    else if (*imode == EXT2_S_IFDIR && *ftype != EXT2_FT_DIR){
        *ftype = EXT2_FT_DIR;
        return 1;
    }
    return 0;
}


int checker_per_file(struct ext2_dir_entry *entry){
    struct ext2_inode *cur_inode = get_inode(entry->inode);
    int fix_number = 0;
    // B
    if(fix_file_type(&cur_inode->i_mode,&entry->file_type) != 0){
        // print "Fixed: Entry type vs inode mismatch: inode [I]", where I is the inode number for the respective file system object. Each inconsistency counts towards to total number of fixes.

        fix_number ++;
    }

    // C
    if (get_inode_bitmap()[entry->inode] & 0){
        get_inode_bitmap()[entry->inode] |= 1;
        fix_number ++;
        //print "Fixed: inode [I] not marked as in-use", where I is the inode number. Each inconsistency counts towards to total number of fixes.
    }

    // D
    if (cur_inode->i_dtime != 0){
        cur_inode->i_dtime = 0;
        fix_number ++;
        // print "Fixed: valid inode marked for deletion: [I]", where I is the inode number. Each inconsistency counts towards to total number of fixes.
    }

    // E
    int i;
    for (i=0; i< (cur_inode->i_blocks*512/EXT2_BLOCK_SIZE);i++){
        if (get_block_bitmap()[cur_inode->i_block[i]] & 0){
            get_block_bitmap()[cur_inode->i_block[i]]  |= 1;
            fix_number ++;
            // print
        }
    }



    return fix_number;
}

int loop_over_file_fix(struct ext2_inode *root){
    int fix_number = 0;
    unsigned int block_no = root->i_blocks*512/EXT2_BLOCK_SIZE;
    unsigned char * root_block = get_block(root->i_block[0]);
    struct ext2_dir_entry * current_entry = (struct ext2_dir_entry *)root_block;
    while (current_entry->name[0] == '.'){
        current_entry = ((void*)root_block + current_entry->rec_len);
    }
    struct ext2_dir_entry *first_entry = current_entry;
    int i;
    for (i=0;i<block_no;i++){
        int count = 0;
        while (count < EXT2_BLOCK_SIZE){
            current_entry = (void*)first_entry + count;
            if (current_entry->file_type & EXT2_FT_DIR){
                struct ext2_inode *current_inode = get_inode(current_entry->inode);
                fix_number += loop_over_file_fix(current_inode);
            }else{
                fix_number += checker_per_file(current_entry);
            }
            count = current_entry->rec_len;
        }
    }
    return fix_number;
}

int fix_files(){
    return loop_over_file_fix(get_root_inode());
}


int counter_fixer(unsigned int* counter, unsigned short actual, char* header, char* counter_name){
    int fix_num = 0;
    int old_value = *counter;
    *counter = actual;
    if (old_value < actual){
        fix_num = actual - old_value;
    }else{
        fix_num = old_value - actual;
    }
    fprintf(stdout,"Fixed: %s's %s counter was off by %d compared to the bitmap",header,counter_name,fix_num);
    return fix_num;
}



int fix_counters(){
    int counter = 0;
    int free_block = 0;
    unsigned short free_inode = 0;
    int byte,bit;
    int in_use;
    struct ext2_super_block *sb = get_sb();
    unsigned char *block_bits = get_block_bitmap();
    unsigned char *inode_bits = get_inode_bitmap();
    for (byte = 0; byte < (sb->s_blocks_count) / 8; byte++) {
        for (bit = 0; bit < 8; bit++) {
            in_use = block_bits[byte] & (1 << bit);
            // block is not in used
            if (in_use == 0) {
                free_block ++;
            }
            in_use = inode_bits[byte] & (1 << bit);
            // Inode is not in used
            if (in_use == 0) {
                free_inode ++;
            }
        }
    }
    if (get_bgd()->bg_free_inodes_count != free_inode){
        counter += counter_fixer((unsigned int *)&(get_bgd()->bg_free_inodes_count),
                                 free_inode,"block group","free inodes");
    }
    if (get_bgd()->bg_free_blocks_count != free_block){
        counter += counter_fixer((unsigned int *)&(get_bgd()->bg_free_blocks_count),
                                 free_inode,"block group","free blcoks");
    }
    if (sb->s_free_inodes_count != free_inode){
        counter += counter_fixer(&(sb->s_free_inodes_count),free_inode,"superblock","free inodes");
    }
    if (sb->s_free_blocks_count != free_block){
        counter += counter_fixer(&(sb->s_free_inodes_count),free_inode,"superblock","free blcoks");
    }

    return counter;
}


int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image file name>\n", argv[0]);
        exit(1);
    }
    disk = create_disk(argv[1]);
    int count = 0;
    count += fix_counters();
    count += fix_files();
    return count;
}


