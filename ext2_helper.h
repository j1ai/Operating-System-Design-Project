

#ifndef EXT2_HELPER_H
#define EXT2_HELPER_H

/* all the libraries are included here */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>



unsigned char *create_disk(char *image);

struct ext2_super_block *get_sb();
struct ext2_group_desc *get_bgd();

unsigned char *get_block_bitmap();
unsigned char *get_inode_bitmap();
unsigned char *get_inode_table();
struct ext2_inode *get_root_inode();
struct ext2_inode *get_inode(int ind);
unsigned char *get_block(int ind);

struct ext2_dir_entry *create_entry(struct ext2_dir_entry *cur_entry, int sizeneeded);
struct ext2_dir_entry *find_sub_entry(struct ext2_inode *c_inode, char *dir_name);
struct ext2_dir_entry *find_entry(const char *path);

unsigned short fourb_alignment (unsigned short rec_len);
unsigned int get_new_block_no();
unsigned int get_new_inode_no ();


char * get_parent_path(const char *path);
char *get_base_name(const char *path);
int get_blocks_number(struct ext2_inode *inode);
void update_iblocks(struct ext2_inode *inode);

#endif