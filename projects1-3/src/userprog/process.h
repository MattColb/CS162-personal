#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include <list.h>
#include <stdint.h>
#include "threads/synch.h"

// At most 8MB can be allocated to the stack
// These defines will be used in Project 2: Multithreading
#define MAX_STACK_PAGES (1 << 11)
#define MAX_ARGS 1024
#define MAX_THREADS 127

/* PIDs and TIDs are the same type. PID should be
   the TID of the main thread of the process */
typedef tid_t pid_t;
typedef char lock_t;
typedef char sema_t;

/* Thread functions (Project 2: Multithreading) */
typedef void (*pthread_fun)(void*);
typedef void (*stub_fun)(pthread_fun, void*);

struct pages {
   bool is_free[128];
   struct lock lock;
};

/* The process control block for a given process. Since
   there can be multiple threads per process, we need a separate
   PCB from the TCB. All TCBs in a process will have a pointer
   to the PCB, and the PCB will have a pointer to the main thread
   of the process, which is `special`. */
struct process {
  /* Owned by process.c. */
  struct wait_status* wait_status; //Process' completion statuss
  struct list children;       // Completion status of children
  uint32_t* pagedir;          /* Page directory. */
  char process_name[16];      /* Name of the main thread */
  struct file* bin_file;      //Executable
  struct thread* main_thread; /* Pointer to main thread */

  struct list fds; //List of file descriptors 
  int next_handle; //Next handle value

  struct semaphore* user_semas[256];
  struct lock* user_locks[256];
  struct list thread_joins;
  struct list thread_list;

  struct pages* pages;

  int next_sema;
  int next_lock;

  struct list fds;
  int next_handle;

  struct dir* work_dir;

  bool marked_for_slaughter;
};

struct join_elem {
   tid_t tid;
   bool joined;
   struct semaphore join;
   struct list_elem elem;
};

struct thread_elem{
   struct thread* thread;
   struct list_elem elem;
};

struct lock_{
   lock_t* lid;
   struct lock lock;
   struct list_elem elem;
};

struct sema_elem {
   sema_t* sid;
   int joiner_tid;
   int joinee_tid;
   struct semaphore join;
   struct list_elem elem;
};

struct wait_status {
   struct list_elem elem; // Children list element
   struct lock lock;
   int ref_cnt; // number of parent/children alive {2,1,0}
   pid_t pid;
   int exit_code;
   struct semaphore dead; //1=alive, 0=dead
};

struct file_descriptor {
   struct list_elem elem; //List element
   struct file* file; //File
   int handle; //File handle
   bool is_dir;
   struct dir* directory;
};

void userprog_init(void);

pid_t process_execute(const char* file_name);
int process_wait(pid_t);
void process_exit(void);
void process_activate(void);

bool is_main_thread(struct thread*, struct process*);
pid_t get_pid(struct process*);

tid_t pthread_execute(stub_fun, pthread_fun, void*);
tid_t pthread_join(tid_t);
void pthread_exit(void);
void pthread_exit_main(void);

#endif /* userprog/process.h */
