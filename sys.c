/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>
#include <utils.h>
#include <io.h>
#include <mm.h>
#include <mm_address.h>
#include <sched.h>
#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

int check_fd(int fd, int permissions) {
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall() {
	return -38; /*ENOSYS*/
}

int sys_getpid() {
	return current()->PID;
}

int ret_from_fork() {
	return 0;
}

int sys_fork() {
    
  if (list_empty(&freequeue))
    return -ENOMEM;

  struct list_head *lhcurrent = list_first(&freequeue);
  list_del(lhcurrent);

  union task_union *uchild = (union task_union*)list_head_to_task_struct(lhcurrent);
  
  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));
  
  /* new pages dir */
  allocate_DIR((struct task_struct*)uchild);
  
  /* Allocate pages for DATA+STACK */
  int new_ph_pag, pag, i;
  page_table_entry *process_PT = get_PT(&uchild->task);
  for (pag = 0; pag < NUM_PAG_DATA; pag++) {
    new_ph_pag = alloc_frame();
    
    if (new_ph_pag!=-1) {
      set_ss_pag(process_PT, PAG_LOG_INIT_DATA + pag, new_ph_pag);
    } else {
      for (i=0; i<pag; i++) {
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA + i));
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA + i);
      }

      list_add_tail(lhcurrent, &freequeue);
      
      return -EAGAIN; 
    }
  }

  page_table_entry *parent_PT = get_PT(current());
  for (pag = 0; pag < NUM_PAG_KERNEL; pag++)
    set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));

  for (pag = 0; pag < NUM_PAG_CODE; pag++)
    set_ss_pag(process_PT, PAG_LOG_INIT_CODE + pag, get_frame(parent_PT, PAG_LOG_INIT_CODE + pag));
  
  for (pag = NUM_PAG_KERNEL + NUM_PAG_CODE; pag < NUM_PAG_KERNEL + NUM_PAG_CODE + NUM_PAG_DATA; pag++) {
    set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
    copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
    del_ss_pag(parent_PT, pag+NUM_PAG_DATA);
  }
  
  set_cr3(get_DIR(current()));
  
  uchild->task.PID = numPID++;
  uchild->task.state = ST_READY;
  
  int register_ebp;
  __asm__ __volatile__ (
    "movl %%ebp, %0\n\t"
    : "=g" (register_ebp)
    :
  );

  register_ebp = (register_ebp - (int)current()) + (int)(uchild);

  uchild->task.kernel_esp = register_ebp + sizeof(DWord);
  
  DWord temp_ebp=*(DWord*)register_ebp;
  
  uchild->task.kernel_esp -= sizeof(DWord);
  *(DWord*)(uchild->task.kernel_esp) = (DWord)&ret_from_fork;
  uchild->task.kernel_esp -= sizeof(DWord);
  *(DWord*)(uchild->task.kernel_esp) = temp_ebp;

  init_stats(&(uchild->task.proc_stats));

  uchild->task.state = ST_READY;
  list_add_tail(&(uchild->task.list), &readyqueue);
  
  return uchild->task.PID;
}

void sys_exit() {
  int i;

  page_table_entry *process_PT = get_PT(current());

  // Deallocate all the propietary physical pages
  for (i = 0; i < NUM_PAG_DATA; i++) {
    free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA + i));
    del_ss_pag(process_PT, PAG_LOG_INIT_DATA + i);
  }
  
  /* Free task_struct */
  list_add_tail(&(current()->list), &freequeue);
  
  current()->PID=-1;
  
  /* Restarts execution of the next process */
  sched_next_rr();
}

int sys_get_stats(int pid, struct stats *st) {
  int i;
  
  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats)))
    return -EFAULT; 
  
  if (pid < 0)
    return -EINVAL;

  for (i = 0; i < NR_TASKS; i++) {
    if (task[i].task.PID == pid) {
      task[i].task.proc_stats.remaining_ticks = ticks_to_leave;
      copy_to_user(&(task[i].task.proc_stats), st, sizeof(struct stats));
      return 0;
    }
  }

  return -ESRCH;
}