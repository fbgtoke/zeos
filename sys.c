/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#define LECTURA 0
#define ESCRIPTURA 1

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

int ret_from_fork() {
	return 0;
}

int sys_fork()
{
  // creates the child process

  int PID=-1;

  // a) Get free task_struct (if no available, return error)
  if (list_empty(&freequeue))
    return -1;

  struct list_head *task_header = list_first(&freequeue);
  struct task_struct* task = list_head_to_task_struct(task_header);
  list_del(task_header);

  // b) Inherit system data
  copy_data((union task_union*) current(), (union task_union*) task, PAGE_SIZE);

  // c) Initialize directories
  allocate_DIR(task);
	copy_data(get_PT(current()), get_PT(task), (NUM_PAG_CODE + PAG_LOG_INIT_CODE) * sizeof(page_table_entry));

  // d) Search physical pages in which to map logical pages for data+stack of the child process
  int i;
  int allocated_frames[NUM_PAG_DATA];
  for (i = 0; i < NUM_PAG_DATA; ++i) {
    allocated_frames[i] = alloc_frame();

    if (allocated_frames[i] == -1) { // Error
      int j;
      for (j = 0; j <= i; ++i) {
        free_frame(allocated_frames[j]);
      }
      return -1;
    }
  }

  // e) Inherit user data
  //    i) Create new address space
  page_table_entry* parent_page_table = get_PT(current());
  page_table_entry* child_page_table = get_PT(task);
  //    ii) Copy data+stack from parent to child

  for (i = 0; i < NUM_PAG_DATA; ++i) {
    set_ss_pag(parent_page_table, (unsigned) (PAG_LOG_INIT_DATA + NUM_PAG_DATA + i), (unsigned) allocated_frames[i]);
    set_ss_pag(child_page_table, (unsigned) (PAG_LOG_INIT_DATA + i), (unsigned) allocated_frames[i]);
  }

  copy_data((void*) (PAG_LOG_INIT_DATA << 12), (void*) ((PAG_LOG_INIT_DATA + NUM_PAG_DATA) << 12), NUM_PAG_DATA * PAGE_SIZE);

  for (i = 0; i < NUM_PAG_DATA; ++i) {
    del_ss_pag(parent_page_table, (unsigned int) PAG_LOG_INIT_DATA + NUM_PAG_DATA + i);
  }

	/* flush TLB */
	set_cr3(get_DIR(current()));

  // f) Assign PID
  static int num_pids = 1;
  PID = ++num_pids;

  // g) Initialize fields of task_struct that are not common
	union task_union* task_union = (union task_union*) task;
	task_union->stack[KERNEL_STACK_SIZE - 1] = 0;
	task_union->stack[KERNEL_STACK_SIZE - 2] = (int)&ret_from_fork;
	task->kernel_esp = KERNEL_ESP(task_union);

	list_add(&(task->list), &readyqueue);

	task->state = ST_READY;

  return PID;
}

void sys_exit()
{  
}
