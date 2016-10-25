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
	printk("IDLE    ");
	}
}

void init_idle (void) {
	struct list_head* free_list_head = list_first(&freequeue);
	list_del(free_list_head); // pop de la queue

	struct task_struct* free_task = list_head_to_task_struct(free_list_head);
	free_task->PID = 0;

	allocate_DIR(free_task);
	idle_task = free_task;

	// 1) Store in the stack of the idle process the address of the code that it will execute (address of the cpu_idle function).
	// 2) Store in the stack the initial value that we want to assign to register ebp when undoing the dynamic link (it can be 0)
	// 3) keep (in a field of its task_struct) the position of the stack where we have stored the initial value for the ebp register.

	union task_union* idle_task_union = (union task_union*) idle_task;
	idle_task_union->stack[KERNEL_STACK_SIZE - 1] = 0; // Dummy for %ebp
	idle_task_union->stack[KERNEL_STACK_SIZE - 2] = (int)&cpu_idle;

	idle_task->kernel_esp = (int) &(idle_task_union->stack[KERNEL_STACK_SIZE - 2]);
}

void init_task1(void) {
	struct task_struct* init = current();

	init->PID = 1;
	allocate_DIR(init);
	set_user_pages(init);

	union task_union* init_union = (union task_union*) init;
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

void task_switch(union task_union *t) {
	__asm__ __volatile__(
		"pushl %esi\n\t"
		"pushl %edi\n\t"
		"pushl %ebx\n\t"
		"pushl 8(%ebp)\n\t"
		"call inner_task_switch\n\t"
		"popl %ebx\n\t"
		"popl %edi\n\t"
		"popl %esi\n\t"
	);
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

	struct task_struct* cur = current();

	tss.esp0 = (int) &(t->stack[KERNEL_STACK_SIZE]); // 1
	set_cr3(t->task.dir_pages_baseAddr); // 2
	__asm__ __volatile__("movl %%ebp, (%0)" : : "g" (&(t->task.kernel_esp))); // 3
	__asm__ __volatile__("movl (%0), %%ebp" : : "g" (&(cur->kernel_esp))); // 4
	__asm__ __volatile__("popl %ebp"); // 5
}

