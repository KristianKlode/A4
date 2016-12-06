/*
 * Threading system.
 */

#ifndef KUDOS_KERNEL_THREAD_H
#define KUDOS_KERNEL_THREAD_H

#include "lib/types.h"
#include <cswitch.h>
#include <pagetable.h>
#include <_thread.h>
#include "proc/process.h"
#include "kernel/types.h"   // TID_t

/* Thread ID data type (index in thread table) */
typedef enum {
  THREAD_FREE,
  THREAD_RUNNING,
  THREAD_READY,
  THREAD_SLEEPING,
  THREAD_NONREADY,
  THREAD_DYING,
} thread_state_t;

#define IDLE_THREAD_TID 0
#define THREAD_FLAG_USERMODE  0x1
#define THREAD_FLAG_ENTERUSER   0x2

/* thread table data structure */
typedef struct {
  /* context save areas context and user_context*/
  /* for interrupts */
  context_t *context;
  /* for traps (syscalls), if applicable */
  context_t *user_context;

  /* thread state */
  thread_state_t state;
  /* which resource this thread sleeps on (0 for none) */
  uint32_t sleeps_on;
  /* pointer to this thread's pagetable */
  pagetable_t *pagetable;

  /* PID. Currently not used for anything, but might be useful
     if process table is implemented. */
  pid_t pid;
  /* pointer to the next thread in list (<0 = end of list) */
  TID_t next;

  /* Attributes */
  uint32_t attribs;

  /* Internal thread structure */
  _kthread_t thread_data;

  /* Return value */
  uint64_t retval;

} thread_table_t;

/* function prototypes */
void thread_table_init(void);
TID_t thread_create(void (*func)(uint64_t), uint64_t arg);
void thread_run(TID_t t);

TID_t thread_get_current_thread(void);
thread_table_t *thread_get_thread_entry(TID_t);
thread_table_t *thread_get_current_thread_entry(void);

void thread_switch(void);
#define thread_yield thread_switch

void thread_goto_userland(context_t *usercontext);

void thread_finish(void);

void thread_sleep_forever(void);

#endif // KUDOS_KERNEL_THREAD_H
