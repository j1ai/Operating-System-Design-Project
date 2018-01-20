#include "ext2.h"
#include "ext2_helper.h"

unsigned char *disk;

int main(int argc, char **argv) {

    if(argc != 5 && argc != 4) {
        fprintf(stderr, "Usage: %s <image file name> <absolute path on this image> <path to be linked>\n", argv[0]);
        exit(1);
    }

    if (argc == 5 && (strcmp(argv[2],"-s") != 0)){
        fprintf(stderr, "Usage: %s <image file name> -s <absolute path on this image> <path to be linked> \n", argv[0]);
        exit(1);
    }
    char *src_path;
    char *target_link_path;
    char* target_name;
    if(argc == 4){
        src_path = argv[2];
        target_link_path = argv[3];
        target_name = basename(argv[3]);
    }else{
        src_path = argv[3];
        target_link_path = argv[4];
        target_name = basename(argv[4]);
    }
    char *image = argv[1];
    disk = create_disk(image);

    char *target_parent_path = get_parent_path(target_link_path);
    int is_symbolic = (argc == 5);

    struct ext2_dir_entry *source_entry = find_entry(src_path);
    struct ext2_dir_entry *target_entry = find_entry(target_link_path);
    struct ext2_dir_entry *target_parent_entry = find_entry(target_parent_path);

    if (target_entry != NULL){
        printf("This destination link already exists\n");
        return EEXIST;
    }

    if (source_entry == NULL){
        printf("The source path does not exists. We would't be able to locate the file\n");
        return ENOENT;
    }

    if (source_entry->file_type & EXT2_S_IFDIR){
        printf("Illegal operation to link a directory\n");
        return EISDIR;
    }

    if(target_parent_entry == NULL){
        printf("Target directory does not exist.\n");
        return ENOENT;
    }


    target_entry = create_entry(target_parent_entry, sizeof(struct ext2_dir_entry)+strlen(target_name));
    if(is_symbolic){
        int target_inode_no = get_new_inode_no();
        struct ext2_inode *target_inode = get_inode(target_inode_no);
        target_entry->inode = (unsigned int)target_inode_no;
        target_entry->file_type = EXT2_FT_SYMLINK;
        target_entry->name_len =(unsigned char) strlen(target_name);
        strcpy(target_entry->name,target_name);
        target_inode->i_block[0] =  get_new_block_no();
        strcpy((char *) get_block(target_inode->i_block[0]),src_path);
        update_iblocks(target_inode);
    }else{
        target_entry->inode = source_entry->inode;
        get_inode(target_entry->inode)->i_links_count++;
    }
    return 0;
}