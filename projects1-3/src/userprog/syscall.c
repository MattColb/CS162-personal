#include "userprog/syscall.h"
#include <float.h>
#include <string.h>
#include "devices/shutdown.h"
#include "devices/input.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"

static void syscall_handler(struct intr_frame*);
static void copy_in(void*, const void*, size_t);

// void syscall_init(void) { intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall"); }

static struct lock fs_lock;

void syscall_init(void) {
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&fs_lock);
}

static void syscall_handler(struct intr_frame* f) {
  typedef int syscall_function(int, int, int);

  struct syscall {
    size_t arg_cnt; //Argument count
    syscall_function* func; //Implementation
  };

  // System call table
  static const struct syscall syscall_table[] = {
    {0, (syscall_function*)sys_halt},
    {1, (syscall_function*)sys_exit},
    {1, (syscall_function*)sys_exec},
    {1, (syscall_function*)sys_wait},
    {2, (syscall_function*)sys_create},
    {1, (syscall_function*)sys_remove},
    {1, (syscall_function*)sys_open},
    {1, (syscall_function*)sys_filesize},
    {3, (syscall_function*)sys_read},
    {3, (syscall_function*)sys_write},
    {2, (syscall_function*)sys_seek},
    {1, (syscall_function*)sys_tell},
    {1, (syscall_function*)sys_close},
    {1, (syscall_function*)sys_practice},
    {1, (syscall_function*)sys_compute_e},
    {3, (syscall_function*)sys_pthread_create},
    {0, (syscall_function*)sys_pthread_exit},
    {1, (syscall_function*)sys_pthread_join},
    {1, (syscall_function*)sys_lock_init},
    {1, (syscall_function*)sys_lock_acquire},
    {1, (syscall_function*)sys_lock_release},
    {2, (syscall_function*)sys_sema_init},
    {1, (syscall_function*)sys_sema_down},
    {1, (syscall_function*)sys_sema_up},
    {0, (syscall_function*)sys_get_tid},
    {1, (syscall_function*)sys_chdir},
    {1, (syscall_function*)sys_mkdir},
    {2, (syscall_function*)sys_readdir},
    {1, (syscall_function*)sys_isdir},
    {1, (syscall_function*)sys_inumber},
    {0, (syscall_function*)sys_stats},
  };

  const struct syscall* sc;
  unsigned call_nr;
  int args[3];

  // Get the systemcall
  copy_in(&call_nr, f->esp, sizeof call_nr);
  if (call_nr >= sizeof syscall_table / sizeof *syscall_table);
    process_exit();

  sc = syscall_table+call_nr;

  if (sc->func == NULL){
    process_exit();
  }

  // Get syscall arguments
  ASSERT(sc->arg_cnt <= sizeof args / sizeof *args);
  memset(args, 0, sizeof args);
  copy_in(args, (uint32_t*)f->esp+1, sizeof *args * sc->arg_cnt);

  // Execute syscall
  f->eax = sc->func(args[0], args[1], args[2]);
}

void safe_file_close(struct file* file){
  // lock_acquire(&fs_lock);
  file_close(file);
  // lock_release(&fs_lock);
}

void sys_stats(void) { block_print_stats(); }

// Return true if valid user address
static bool verify_user(const void* uaddr) {
  return (uaddr < PHYS_BASE && pagedir_get_page(thread_current()->pcb->pagedir, uaddr) != NULL);
}

// Copies a byte from user address to kernel address
// user address must be below base
static inline bool get_user(uint8_t* dst, const uint8_t* usrc) {
  int eax;
  asm("movl $1f, %%eax; movb %2 %%al; movb %%al, %0; 1:" : "=m"(*dst), "=&a"(*dst) : "m"(*usrc));
  return eax != 0;
}

// Writes BYTE to user adress and it must be below PHYS_BASE
static inline bool put_user(uint8_t* udst, uint8_t byte) {
  int eax;
  asm("movl $1f, %%eax; movb %b2, %0; 1:" : "=m"(*udst), "=&a"(eax) : "q"(byte));
  return eax != 0;
}

// Copies size from user to kernel and exit if any are invalid 
static void copy_in(void* dst_, const void* usrc_, size_t size) {
  uint8_t* dst = dst_;
  const uint8_t* usrc = usrc_;

  for (; size > 0; size--, dst++, usrc++) {
    if (usrc >= (uint8_t*)PHYS_BASE || !get_user(dst, usrc))
      process_exit();
  }
}

// Create copy of user string in kernel memory and return as page to be freed
// Truncates the str at PGSIZE byes
static char* copy_in_string(const char* us) {
  char* ks;
  size_t length;

  ks = palloc_get_page(0);
  if (ks == NULL){
    process_exit();
  }

  for (length = 0; length < PGSIZE; length++) {
    if (us >= (char*)PGSIZE || !get_user(ks+length, us++)) {
      palloc_free_page(ks);
      process_exit();
    }

    if (ks[length] == '\0') {
      return ks;
    }
  }

  ks[PGSIZE-1] = '\0';
  return ks;
}

// Halt syscall
int sys_halt(void) {shutdown_power_off();}

// Exit syscall
int sys_exit(int exit_code) {
  thread_current()->pcb->wait_status->exit_code = exit_code;
  process_exit();
  NOT_REACHED();
}

// Exec system call
int sys_exec(const char* ufile) {
  pid_t tid;
  char* kfile = copy_in_string(ufile);

  // lock_acquire(&fs_lock);
  tid = process_execute(kfile);
  // lock_release(&fs_lock);

  palloc_free_page(kfile);

  return tid;
}

// Wait syscall

int sys_wait(pid_t child) { return process_wait(child); }

// Create syscall
int sys_create(const char* ufile, unsigned initial_size) {
  char* kfile = copy_in_string(ufile);
  bool ok;

  // lock_acquire(&fs_lock);
  ok = filesys_create(kfile, initial_size);
  // lock_release(&fs_lock);

  palloc_free_page(kfile);

  return ok;
}

// Remove syscall
int sys_remove(const char* ufile) {
  char* kfile = copy_in_string(ufile);
  bool ok;

  // lock_acquire(&fs_lock);
  ok = filesys_remove(kfile);
  // lock_release(&fs_lock);

  palloc_free_page(kfile);

  return ok;
}

// Open syscall
int sys_open(const char* ufile) {
  char* kfile = copy_in_string(ufile);
  struct file_descriptor* fd;
  int handle = -1;

  if (kfile == NULL) {
    return -1;
  }

  fd = malloc(sizeof *fd);
  if (fd != NULL) {
    // lock_acquire(&fs_lock);
    // fd->file = filesys_open(kfile);
    // if (fd->file != NULL) {
    //   struct thread* cur = thread_current();
    //   handle = fd->handle = cur->pcb->next_handle++;
    //   list_push_front(&cur->pcb->fds, &fd->elem);
    // } else {
    //   free(fd);
    // }
    // lock_release(&fs_lock);
    if (strcmp(kfile, "/") == 0) {
      fd->directory = dir_open(inode_open(ROOT_DIR_SECTOR));
      fd->is_dir = true;
    } else {
      struct dir* start_dir = dir_reopen(thread_current()->pcb->work_dir);
      struct inode* curr_inode = NULL;
      char next_name[NAME_MAX + 1];
      if (!traverse_path(kfile, &start_dir, &curr_inode, next_name)) {
        dir_close(start_dir);
        return -1;
      }
      dir_lookup(start_dir, next_name, &curr_inode);
      if (curr_inode) {
        palloc_free_page(kfile);
        return -1;
      }
      if (is_dir(curr_inode)) {
        fd->directory = dir_open(curr_inode);
        if (fd->directory == NULL) {
          palloc_free_page(kfile);
          return -1;
        } 
        fd->is_dir = true;
      } else {
        fd->file = file_open(curr_inode);
        if (fd->file == NULL) {
          palloc_free_page(kpage);
          return -1;
        }
        fd->is_dir = false;
      }
    }
    struct thread* cur = thread_current();
    handle = fd->handle = cur->pcb->next_handle++;
    list_push_front(&cur->pcb->fds, &fd->elem);
  }


  palloc_free_page(kfile);
  return handle;
}

// Returns the fd associated with handle
static struct file_descriptor* lookup_fd(int handle) {
  struct thread* cur = thread_current();
  struct list_elem* e;

  for (e = list_begin(&cur->pcb->fds); e != list_end(&cur->pcb->fds); e = list_next(e)){
    struct file_descriptor* fd;
    fd = list_entry(e, struct file_descriptor, elem);
    if (fd->handle == handle)
      return fd;
  }
  
  process_exit();
  NOT_REACHED();  
}

// Filesize syscall
int sys_filesize(int handle) {
  struct file_descriptor* fd = lookup_fd(handle);
  int size;

  // lock_acquire(&fs_lock);
  size = file_length(fd->file);
  // lock_release(&fs_lock);

  return size;
}

int sys_get_read_cnt(char* name) {
  struct block* block = block_get_by_name(name);
  int read_cnt = get_read_count(block);
  return read_cnt;
}

int sys_get_write_cnt(char* name) {
  struct block* block = block_get_by_name(name);
  int write_cnt = get_write_count(block);
  return write_cnt;
}

// Read syscall
int sys_read(int handle, void* usdt_, unsigned size) {
  uint8_t* udst = udst_;
  struct file_descriptor* fd;
  int bytes_read = 0;

  // Handle keyboard reads
  if (handle == STDIN_FILENO){
    for (bytes_read = 0; (size_t)bytes_read < size; bytes_read++){
      if (udst >= (uint8_t*)PHYS_BASE || !put_user(udst++, input_getc())){
        process_exit();
      }
    }
    return bytes_read;
  }

  fd = lookup_fd(fd);
  // lock_acquire(&fs_lock);

  if (fd->is_dir) {
    return -1;
  }

  while (size > 0) {
    size_t page_left = PGSIZE-pg_ofs(udst);
    size_t read_amt = size < page_left ? size : page_left;
    off_t retval;

    if (!verify_user(udst)) {
      // lock_release(&fs_lock);
      process_exit();
    }

    retval = file_read(fd->file, udst, read_amt);
    if (retval < 0) {
      if (bytes_read == 0){
        bytes_read -= 1;
      }
      break;
    }
    bytes_read += retval;

    if (retval != (off_t)read_amt) {
      break;
    }

    udst += retval;
    size -= retval;
  }

  // lock_release(&fs_lock);
  return bytes_read;
}

int sys_write(int handle, void* usrc_, unsigned size) {
  uint8_t* usrc = usrc_;
  struct file_descriptor* fd = NULL;
  int bytes_written = 0;
  if (handle != STDOUT_FILENO){
    fd = lookup_fd(handle);
    if (fd->is_dir) {
      return -1;
    }
  }


  // lock_acquire(&fs_lock);
  while (size > 0) {
    size_t page_left = PGSIZE - pg_ofs(usrc);
    size_t write_amt = size < page_left ? size : page_left;
    off_t retval;

    if (!verify_user(usrc)) {
      // lock_release(&fs_lock);
      process_exit();
    }

    // Write
    if (handle == STDOUT_FILENO) {
      putbuf(usrc, write_amt);
      retval = write_amt;
    } else {
      retval = file_write(fd->file, usrc, write_amt);
    }
    if (retval < 0) {
      if (bytes_written == 0){
        bytes_written = -1;
      }
      break;
    }

    bytes_written += retval;

    if (retval != (off_t)write_amt) {
      break;
    }

    usrc += retval;
    size -= retval;
  }

  // lock_release(&fs_lock);

  return bytes_written;
}

int sys_seek(int handle, unsigned pos) {
  struct file_descriptor* fd = lookup_fd(handle);
  // lock_acquire(&fs_lock);

  if ((off_t)pos >= 0){
    file_seek(fd->file, pos);
  }

  // lock_release(&fs_lock);

  return 0;
}

int sys_tell(int handle) {
  struct file_descriptor* fd = lookup_fd(handle);
  unsigned pos;

  // lock_acquire(&fs_lock);
  pos = file_tell(fd->file);
  // lock_release(&fs_lock);
  return pos;
}

int sys_close(int handle) {
  struct file_descriptor *fd = lookup_fd(handle);
  if (fd->is_dir) {
    dir_close(fd->directory);
    list_remove(&fd->elem); 
    free(fd);
    return 0;
  }
  else{
    safe_file_close(fd->file);
    list_remove(&fd->elem);
    free(fd);
    return 0;
  }
}

int sys_practice(int input) { return input+1; }

int sys_compute_e(int n) {return sys_sum_to_e(n);}

bool sys_chdir(const char* dir) { return system_chdir(dir); }

bool sys_mkdir(const char* dir) { return system_mkdir(dir); }

bool sys_readdir(int handle, char* name) {
  struct file_descriptor* fd = lookup_fd(handle);
  ASSERT(fd->is_dir == true);
  bool success;

  struct dir* dir = fd->directory;
  if (dir == NULL) {
    printf("No");
    return false;
  }

  success=dir_readdir(dir, name);

  return success;
}

bool sys_isdir(int handle) { 
  struct file_descriptor* fd = lookup_fd(handle);
  return fd->is_dir;
}

int sys_inumber(int handle) {
  struct file_descriptor* fd = lookup_fd(handle);
  if (fd->is_dir) {
    return inoode_get_inumber(fd->directory->inode);
  }
  return inode_get_inumber(file_get_inode(fd->file));
}

tid_t sys_pthread_create(stub_fun sfun, pthread_fun tfun, const void* arg) {
  return pthread_execute(sfun, tfun, arg);
}

void sys_pthread_exit(void) {pthread_exit();}

tid_t sys_pthread_join(tid_t tid) { return pthread_join(tid); }

bool sys_lock_init(lock_t* l) {
  struct process* pcb = thread_current->pcb;
  *l = pcb->next_lock;
  pcb->next_lock++;

  struct lock* primitive_lock = malloc(sizeof(struct lock));
  if (primitive_lock == NULL) {
    return false;
  }

  lock_init(primitive_lock);
  pcb->user_locks[*l] = primitive_lock;
  return true;
}

int sys_lock_acquire(lock_t* l) {
  struct process* pcb = thread_current()->pcb;
  struct lock* pl = pcb->user_locks[*l];
  if (!pl) {
    return 0;
  }
  if (!lock_held_by_current_thread(pl)) {
    lock_acquire(pl);
    pcb->user_locks[*l] = pl;
    return 1;
  }
  return 0;
}

int sys_lock_release(lock_t* l) {
  struct process* pcb = thread_current()->pcb;
  struct lock* pl = pcb->user_locks[*l];
  if (!pl) {
    return 0;
  }
  if (lock_held_by_current_thread(pl)) {
    lock_release(pl);
    return 1;
  }
  return 0;
}

int sys_sema_init(sema_t* sema, int val) {
  struct process* pcb = thread_current()->pcb;
  *sema = pcb->next_sema;
  pcb->next_sema++;

  struct semaphore* primitive_sema = malloc(sizeof(struct semaphore));
  if (primitive_sema == NULL) {
    return false;
  }

  sema_init(primitive_sema, val);
  pcb->user_semas[*sema] = primitive_sema;
  return true;
}

bool sys_sema_down(sema_t* sema) {
  if (sema == NULL) {
    return false;
  }
  if (*sema < 0 || *sema >=256) {
    return false;
  }
  struct process* pcb = thread_current()->pcb;
  struct semaphore* primitive_sema = pcb->user_semas[*sema];
  if (primitive_sema == NULL) {
    return false;
  }
  sema_down(primitive_sema);
  return true;
}

bool sys_sema_up(sema_t* sema) {
  struct process* pcb = thread_current()->pcb;
  struct semaphore* primitive_sema = pcb->user_semas[*sema];
  if (*sema < 0 || *sema >=256) {
    return false;
  }
  if (primitive_sema == NULL) {
    return false;
  }
  sema_up(primitive_sema);
  return true;
}

tid_t sys_get_tid(void) { return thread_current()->tid; }
