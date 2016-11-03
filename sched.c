/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */
int numPID = 1;

#if 1
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
#endif

extern struct list_head blocked;
struct task_struct* idle_task;


/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t) 
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{
	;
	}
}

void init_idle (void) {
	struct list_head* free_list_head = list_first(&freequeue);
	list_del(free_list_head); // pop de la queue

	struct task_struct* free_task = list_head_to_task_struct(free_list_head);
	free_task->PID = 0;
	free_task->quantum = QUANTUM;
	
	init_stats(&(free_task->proc_stats));
	allocate_DIR(free_task);

	// 1) Store in the stack of the idle process the address of the code that it will execute (address of the cpu_idle function).
	// 2) Store in the stack the initial value that we want to assign to register ebp when undoing the dynamic link (it can be 0)
	// 3) keep (in a field of its task_struct) the position of the stack where we have stored the initial value for the ebp register.

	union task_union* free_union = (union task_union*) free_task;
	free_union->stack[KERNEL_STACK_SIZE - 1] = (unsigned long)&cpu_idle;
	free_union->stack[KERNEL_STACK_SIZE - 2] = 0; // Dummy for %ebp

	free_task->kernel_esp = (int) &(free_union->stack[KERNEL_STACK_SIZE - 2]);

	idle_task = free_task;
}

void init_task1(void) {
	struct list_head *free_list_head = list_first(&freequeue);
  	list_del(free_list_head);

	struct task_struct *init = list_head_to_task_struct(free_list_head);
	union task_union* init_union = (union task_union*) init;

	init->PID = numPID++;
	init->state = ST_RUN;
	init->quantum = QUANTUM;
	
	ticks_to_leave = init->quantum;

	init_stats(&init->proc_stats);
	allocate_DIR(init);
	set_user_pages(init);

	tss.esp0 = (DWord)&(init_union->stack[KERNEL_STACK_SIZE]);
	set_cr3(init->dir_pages_baseAddr);
}


void init_sched(){
	// Inicialitzar freequeue i ready	queue
	INIT_LIST_HEAD( &freequeue );
	INIT_LIST_HEAD( &readyqueue );

	int i;
	for(i = 0; i < NR_TASKS; ++i)
		list_add_tail( &(task[i].task.list), &freequeue );

}

struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

void inner_task_switch(union task_union *t) {
	/*
		1) Update TSS to point to t's system stack
		2) Change user address space (set_cr3)
		3) Store current EBP in the PCB
		4) Change system stack by setting ESP to point to the new PCB's EBP value
		5) Restore EBP from stack
		6) ret
	*/

	page_table_entry *new_DIR = get_DIR(&t->task);
	tss.esp0 = (int) &(t->stack[KERNEL_STACK_SIZE]); // 1
	set_cr3(new_DIR); // 2

	__asm__ __volatile__(
		"movl %%ebp, %0\n\t"
		: "=g" (current()->kernel_esp)
		: 
	); // 3

	__asm__ __volatile__(
		"movl %0, %%esp" 
		: 
		: "g" (t->task.kernel_esp)
	); // 4

	__asm__ __volatile__(
		"popl %%ebp"
		:
		:
	); // 5

	__asm__ __volatile__(
		"ret\n\t"
		:
		:
	); // 6
}

void task_switch(union task_union *t) {

	__asm__ __volatile__(
		"pushl %esi\n\t"
		"pushl %edi\n\t"
		"pushl %ebx\n\t"
	);

	inner_task_switch(t);

	__asm__ __volatile__ (
		"popl %ebx\n\t"
		"popl %edi\n\t"
		"popl %esi\n\t"
	);

}


/* Process Scheduling */
void update_sched_data_rr() {
	--ticks_to_leave;
}

int needs_sched_rr() {
	if ((ticks_to_leave == 0)&&(!list_empty(&readyqueue))) return 1;
  	if (ticks_to_leave == 0) ticks_to_leave = current()->quantum;
	return 0;
}

void update_process_state_rr(struct task_struct *t, struct list_head *dest) {
	if (t->state != ST_RUN)
		list_del(&(t->list));

  if (dest != NULL) {
    
		list_add_tail(&(t->list), dest);

    if (dest != &readyqueue) {
			t->state = ST_BLOCKED;
	} else {
		update_stats(&(current()->proc_stats.system_ticks), &(current()->proc_stats.elapsed_total_ticks));
      	t->state = ST_READY;
    }

  } else {
		t->state = ST_RUN;
	}
}

void sched_next_rr() {
	struct list_head *e;
	struct task_struct *t;

	e = list_first(&readyqueue);

	if (e) {
		list_del(e);

		t = list_head_to_task_struct(e);
	} else {
		t = idle_task;
	}

	t->state = ST_RUN;
	ticks_to_leave = get_quantum(t);

	task_switch((union task_union*)t);

	update_stats(&(current()->proc_stats.system_ticks), &(current()->proc_stats.elapsed_total_ticks));
	update_stats(&(t->proc_stats.ready_ticks), &(t->proc_stats.elapsed_total_ticks));
	t->proc_stats.total_trans++;
}

void schedule() {
	update_sched_data_rr();

  	if (needs_sched_rr()) {
	    update_process_state_rr(current(), &readyqueue);
	    sched_next_rr();
	}
}

int get_quantum(struct task_struct* task) {
	return task->quantum;
}

void set_quantum(struct task_struct* task, unsigned int new_quantum) {
	task->quantum = new_quantum;
}


void init_stats(struct stats *s) {
	s->user_ticks = 0;
	s->system_ticks = 0;
	s->blocked_ticks = 0;
	s->ready_ticks = 0;
	s->elapsed_total_ticks = get_ticks();
	s->total_trans = 0;
	s->remaining_ticks = get_ticks();
}

void update_stats(unsigned long *v, unsigned long *elapsed) {
  unsigned long current_ticks;
  
  current_ticks = get_ticks();
  
  *v += current_ticks - *elapsed;
  
  *elapsed=current_ticks;
  
}

struct stats * get_task_stats (struct task_struct* t){
  struct stats * r = &(t->proc_stats);
  return r;
}

struct list_head * get_task_list(struct  task_struct* t){
  struct list_head * r = &(t->list);
  return r;
}
