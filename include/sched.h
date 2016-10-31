/*
 * sched.h - Estructures i macros pel tractament de processos
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <list.h>
#include <types.h>
#include <mm_address.h>
#include <stats.h>
#include <utils.h>

#define NR_TASKS      10
#define KERNEL_STACK_SIZE	1024

#define QUANTUM 100

enum state_t { ST_RUN, ST_READY, ST_BLOCKED };

extern int numPID;
struct task_struct {
  	int PID;			/* Process ID. This MUST be the first field of the struct. */
  	page_table_entry * dir_pages_baseAddr;
	unsigned long kernel_esp;

	struct list_head list; // anchor para la lista

	// Data for scheduling
	int quantum;
	enum state_t state;
	struct stats proc_stats;
};

union task_union {
  struct task_struct task;
  unsigned long stack[KERNEL_STACK_SIZE];    /* pila de sistema, per procés */
};

extern union task_union protected_tasks[NR_TASKS+2];
extern union task_union *task; /* Vector de tasques */
extern struct task_struct *idle_task;

struct list_head freequeue;
struct list_head readyqueue;


#define KERNEL_ESP(t)       	(DWord) &(t)->stack[KERNEL_STACK_SIZE]

#define INITIAL_ESP       	KERNEL_ESP(&task[1])

/* Inicialitza les dades del proces inicial */
void init_task1(void);

void init_idle(void);

void init_sched(void);

struct task_struct * current();

void task_switch(union task_union*t);

struct task_struct *list_head_to_task_struct(struct list_head *l);

int allocate_DIR(struct task_struct *t);

page_table_entry * get_PT (struct task_struct *t) ;

page_table_entry * get_DIR (struct task_struct *t) ;

/* Headers for the scheduling policy */
int ticks_to_leave;

void sched_next_rr();
void update_process_state_rr(struct task_struct *t, struct list_head *dest);
int needs_sched_rr();
void update_sched_data_rr();

void schedule();

int get_quantum(struct task_struct* task);
void set_quantum(struct task_struct* task, unsigned int new_quantum);

/* Headers for stats */
void init_stats(struct stats *s);
void update_stats(unsigned long *v, unsigned long *elapsed);
struct stats * get_task_stats (struct task_struct *t);
struct list_head *get_task_list(struct  task_struct *t);


#endif  /* __SCHED_H__ */
