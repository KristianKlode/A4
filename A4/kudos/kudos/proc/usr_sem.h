/// Kernel support for userland processes.

#ifndef KUDOS_PROC_USR_SEM_H
#define KUDOS_PROC_USR_SEM_H

#include "lib/types.h"
#include "vm/memory.h"
#include "kernel/klock.h"
#include "kernel/semaphore.h"

#define USR_SEM_MAX_SEMS (256)


typedef struct {
  klock_t klock;
  int value;
  int maxvalue;
  char name;
  semaphore_t *ksem;
} usr_sem_t;

#endif // KUDOS_PROC_USR_SEM_H
