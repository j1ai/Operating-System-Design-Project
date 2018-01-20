#include "ext2_helper.h"
#include "ext2.h"

unsigned char *disk;

int main(int argc, char **argv) {
    
    if(argc != 4) {
        fprintf(stderr, "Usage: %s <image file name> <src file> <virtual path> \n", argv[0]);
        exit(1);
    }
    char *image = argv[1];
    char *abs_path = argv[2];
    char *virtual_path = argv[3];
    disk = create_disk(image);
    FILE *fp = fopen(abs_path,"r");
    if (!fp){
        fprintf(stderr, "source file doesn't exist");
        exit(-1);
    }
    fseek(fp, 0L, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    char* parent_path = get_parent_path(virtual_path);
    char* name = get_base_name(abs_path);
    struct ext2_dir_entry *parent_entry = find_entry(parent_path);
    if(parent_entry == NULL){
        printf("parent path doesn't exist");
        return ENOENT;
    }




    struct ext2_dir_entry *target_entry = find_entry(virtual_path);
    if (target_entry != NULL){
        printf("The target already exist.%s\n",target_entry->name);
        return EEXIST;
    }


    // source and destinations are ready.
    unsigned int new_inode_no = get_new_inode_no();
    size_t length = strlen(name);
    unsigned short entry_size = sizeof(struct ext2_dir_entry)+length;
    struct ext2_dir_entry *new_entry = create_entry(parent_entry,entry_size);
    strcpy(new_entry->name,name);
    new_entry->file_type = EXT2_FT_REG_FILE;
    new_entry->inode = new_inode_no;
    new_entry->name_len = (unsigned char)length;

    struct ext2_inode *new_inode = get_inode(new_entry->inode);
    new_inode->i_blocks = (unsigned int) file_size/512;
    new_inode->i_size = (unsigned int)file_size;

    unsigned int *blocks = get_inode(new_inode_no)->i_block;
    int i = 0;
    long block_required = file_size / EXT2_BLOCK_SIZE;
    for(i=0;i<12;i++){
        if(i<block_required){
            blocks[i] = get_new_block_no();
            fread(get_block(blocks[i]),1,EXT2_BLOCK_SIZE,fp);
        }else{
            blocks[i]= 0;
        }
    }

    fclose(fp);
    return 0;
}

