#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"

/* Partition that contains the file system. */
struct block* fs_device;

static void do_format(void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void filesys_init(bool format) {
  fs_device = block_get_role(BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC("No file system device found, can't initialize file system.");

  inode_init();
  free_map_init();
  buffer_cache_init();

  if (format)
    do_format();

  free_map_open();
  thread_current()->pcb->work_dir = dir_open_root();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void filesys_done(void) { 
  free_map_close();
  flush_cache();
}

bool traverse_path(const char* path, struct dir** dir, struct inode** cinode, char name[NAME_MAX+1]) {
  if (*path == '\0') {
    return false;
  }

  struct dir* cur_dir =
    (*path == '/') ? dir_open_root() : dir_reopen(thread_current()->pcb->work_dir);

  if (inode_is_removed(cur_dir->inode)) {
    dir_close(cur_dir);
    return false;
  }

  struct inode* inode = inode_reopen(cur_dir->inode);

  while(get_next_part(name, &path)) {
    if (strcmp(name, ".") == 0) {
      continue;
    }
    if (inode == NULL) {
      dir_close(cur_dir);
      return false;
    }
    if (!is_dir(inode)) {
      inode_close(inode);
      dir_close(cur_dir);
      return false;
    } else {
      struct dir* next_dir = dir_open(inode);
      dir_close(cur_dir);
      cur_dir = next_dir;
      dir_lookup(cur_dir, name, &inode);
    }
  }

  *dir = cur_dir;
  *cinode = inode;
  return true;
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool filesys_create(const char* name, off_t initial_size) {
  struct dir* dir = dir_open_root();
  struct inode* cinode = NULL;
  char next_name[NAME_MAX+1];

  if (!traverse_path(name, &dir, &cinode, next_name)) {
    dir_close(dir);
    return false;
  }
  if (cinode != NULL) {
    inode_close(cinode);
    dir_close(dir);
    return false;
  }

  block_sector_t inode_sector = 0;
  bool success = (free_map_allocate(1, &inode_sector) &&
                  inode_create(inode_sector, initial_size) && dir_add(dir, next_name, inode_sector));
  if (!success && inode_sector != 0)
    free_map_release(inode_sector, 1);
    
  dir_close(dir);
  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file* filesys_open(const char* name) {
  struct dir* dir = dir_reopen(thread_current()->pcb->work_dir);
  struct inode* inode = NULL;
  char next_name[NAME_MAX+1];

  if (!traverse_path(name, &dir, &inode, next_name)) {
    dir_close(dir);
    return NULL;
  }
  
  dir_lookup(dir, next_name, &inode);
  return file_open(inode);
}

bool is_empty(struct inode* inode) {
  struct dir_entry e;
  for (size_t ofs = 0; inode_read_at(inode, &e, sizeof e, ofs) == sizeof e; ofs += sizeof e) {
    if (e.in_use) {
      if (strcmp(e.name, ".") != 0 && strcmp(e.name, "..") != 0) {
        return false;
      }
    }
  }
  return true;
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool filesys_remove(const char* name) {
  struct dir* dir;
  char next_name[NAME_MAX+1];
  struct inode* cfile = NULL;
  bool lookup;

  if (!traverse_path(name, &dir, &cfile, next_name)) {
    dir_close(dir);
    return false;
  }

  bool success=true;
  struct inode* inode;
  if (!dir_lookup(dir, next_name, &inode)) {
    dir_close(dir);
    return false;
  }
  if (is_dir(inode) && !is_empty(inode)) {
    dir_close(dir);
    return false;
  }
  struct thread* t = thread_current();
  if (inode_get_inumber(inode) == inode_get_inumber(t->pcb->work_dir->inode)) {
    return false;
  }

  struct list_elem* e;
  struct list* descriptors = &t->pcb->fds;

  for (e = list_begin(descriptors); e != list_end(descriptors); e = list_next(e)) {
    struct file_descriptor* fd;
    fd = list_entry(e, struct file_descriptor, elem);
    if (fd->is_dir && inode_get_inumber(inode) == inode_get_inumber(fd->directory->inode)) {
      return false;
    }
  }

  success = dir_remove(dir, next_name);

  inode_close(cfile);
  dir_close(dir);

  return success;
}

/* Formats the file system. */
static void do_format(void) {
  printf("Formatting file system...");
  free_map_create();
  if (!dir_create(ROOT_DIR_SECTOR, 16))
    PANIC("root directory creation failed");
  free_map_close();
  printf("done.\n");
}

int get_next_part(char part[NAME_MAX+1], const char** srcp) {
  const char* src = *srcp;
  char* dst = part;

  while (*src == '/') {
    src++;
  }
  if (*src == '\0') {
    return 0;
  }

  while (*src != '/' && *src != '\0') {
    if (dst < part+NAME_MAX) {
      *dst++ = *src;
    } else {
      return 1;
    }
    src++;
  }

  *dst = '\0';
  *srcp = src;
  return 1;
}
