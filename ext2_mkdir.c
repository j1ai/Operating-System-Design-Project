#include "ext2.h"
#include "ext2_helper.h"

unsigned char *disk;

int main(int argc, char **argv) {
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <image file name> <absolute path on this image> \n", argv[0]);
        exit(1);
    }

    char *image = argv[1];
    char *target_path = argv[2];
    if (strcmp(target_path,"/") == 0){
        printf("Root Directory already exists, can't make a directory here\n");
        return EEXIST;
    }
    disk = create_disk(image);
    char* parent_path = get_parent_path(target_path);

    struct ext2_dir_entry *parent_entry = find_entry(parent_path);
    if (parent_entry == NULL){
        printf("The parent path does not existst.\n");
        return ENOENT;
    }
    char *target = get_base_name(target_path);
    struct ext2_inode *parent_inode = get_inode(parent_entry->inode);

    struct ext2_dir_entry *target_entry = find_sub_entry(parent_inode, target);

    if (target_entry != NULL){
        printf("The target already exist.\n");
        return EEXIST;
    }
    char* name = get_base_name(target_path);

    target_entry = create_entry(parent_entry,(sizeof(struct ext2_dir_entry)+strlen(name)));
    target_entry->file_type = EXT2_FT_DIR;
    target_entry->name_len = (unsigned char)strlen(name);
    strcpy(target_entry->name, name);
    target_entry->inode = get_new_inode_no();


    struct ext2_inode *target_inode = get_inode(target_entry->inode);

    target_inode->i_block[0] = get_new_block_no();
    target_inode->i_mode = EXT2_S_IFDIR;

    struct ext2_dir_entry *first_entry = (struct ext2_dir_entry *)get_block(target_inode->i_block[0]);
    strcpy(first_entry->name,".");
    first_entry->name_len = 1;
    first_entry->inode = target_entry->inode;
    first_entry->file_type = EXT2_FT_DIR;
    fourb_alignment(sizeof(struct ext2_dir_entry)+ first_entry->name_len);
    first_entry->rec_len = fourb_alignment(sizeof(struct ext2_dir_entry)+ first_entry->name_len);


    struct ext2_dir_entry *second_entry = first_entry + first_entry->rec_len;
    strcpy(second_entry->name,"..");
    second_entry->name_len = 2;
    second_entry->inode = parent_entry->inode;
    second_entry->file_type = EXT2_FT_DIR;
    second_entry->rec_len = (unsigned short) EXT2_BLOCK_SIZE - first_entry->rec_len;

    return 0;

}