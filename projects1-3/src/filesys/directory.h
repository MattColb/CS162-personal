#ifndef FILESYS_DIRECTORY_H
#define FILESYS_DIRECTORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "threads/thread.h"
#include "filesys/inode.h"
#include "threads/malloc.h"
#include "userprog/process.h"
#include "devices/block.h"

/* Maximum length of a file name component.
   This is the traditional UNIX maximum length.
   After directories are implemented, this maximum length may be
   retained, but much longer full path names must be allowed. */
#define NAME_MAX 14

struct dir {
   struct inode* inode;
   off_t pos;
   int num_entries;
};

struct dir_entry {
   block_sector_t inode_sector;
   char name[NAME_MAX+1];
   bool in_use;
};

struct inode;

/* Opening and closing directories. */
bool dir_create(block_sector_t sector, size_t entry_cnt);
struct dir* dir_open(struct inode*);
struct dir* dir_open_root(void);
struct dir* dir_reopen(struct dir*);
void dir_close(struct dir*);
struct inode* dir_get_inode(struct dir*);

/* Reading and writing. */
bool dir_lookup(const struct dir*, const char* name, struct inode**);
bool dir_add(struct dir*, const char* name, block_sector_t);
bool dir_remove(struct dir*, const char* name);
bool dir_readdir(struct dir*, char name[NAME_MAX + 1]);
bool system_chrdir(const char* dir);
bool system_mkdir(const char* dir);

#endif /* filesys/directory.h */
