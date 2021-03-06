/// Kernel support for userland processes.

#include <arch.h>

#include <stddef.h>             // NULL, comes from GCC.

#include "proc/process.h"
#include "proc/elf.h"
#include "kernel/thread.h"
#include "kernel/assert.h"
#include "kernel/interrupt.h"
#include "kernel/config.h"
#include "fs/vfs.h"
#include "kernel/sleepq.h"
#include "vm/memory.h"
#include "kernel/klock.h"
#include "proc/usr_sem.h"
#include "kernel/semaphore.h"
#include "lib/libc.h"

#include "drivers/device.h"     // device_*
#include "drivers/gcd.h"        // gcd_*
#include "kernel/assert.h"      // KERNEL_ASSERT
#include "proc/syscall.h"       // FD_*, IO_*


static usr_sem_t usr_sem_table[USR_SEM_MAX_SEMS];
static klock_t usr_sem_table_lock;

/// Initialize process table and locks.
void usr_sem_init() {
  int i;
  klock_init(&usr_sem_table_lock);
  for (i = 0; i < USR_SEM_MAX_SEMS; ++i) {
    klock_init(&usr_sem_table[i].klock);
    usr_sem_table[i].value = -1;
    usr_sem_table[i].maxvalue = -1;
    usr_sem_table[i].name = "";
    usr_sem_table[i].ksem = -1;
  }
}

usr_sem_t* usr_sem_open(const char* name, int value){
    if (value < 0){
      for ( int i = 0; i < USR_SEM_MAX_SEMS; ++i) {
        if(stringcmp(name,usr_sem_table[i].name)){
          return &usr_sem_table[i];
        }
      return NULL;
      }
    }
    for (int i = 0; i < USR_SEM_MAX_SEMS; ++i) {
      if(stringcmp(name,usr_sem_table[i].name)){
        klock_status_t status = klock_lock(&usr_sem_table_lock);
        usr_sem_table[i].value = value;
        usr_sem_table[i].maxvalue = value;
        usr_sem_table[i].name = name;
        usr_sem_table[i].ksem = semaphore_create(value);
        klock_open(status, &usr_sem_table_lock);
        return &usr_sem_table[i];
      }
    }
}


int usr_sem_close(usr_sem_t* sem){
  if (sem->value != sem->maxvalue){
    return(-1);
  }
  klock_init(sem->klock);
  klock_status_t status = klock_lock(&usr_sem_table_lock);
  semaphore_destroy(sem->ksem);
  sem->value = -1;
  sem->maxvalue = -1;
  sem->name = "";
  sem->ksem = -1;
  klock_open(status, &usr_sem_table_lock);
  return 0;
}

int usr_sem_p(usr_sem_t* sem){
  klock_status_t status = klock_lock(&usr_sem_table_lock);
  //The page fault is below this point.
  if(sem->value < 1){
    klock_open(status, &usr_sem_table_lock);
    return -2;
  }
  //The page fault is above this point.
  sem->value--;
  semaphore_P(sem->ksem);
  klock_open(status, &usr_sem_table_lock);
  return 0;
}

int usr_sem_v(usr_sem_t* sem){
  klock_status_t status = klock_lock(&usr_sem_table_lock);
  if(sem->value > (sem->maxvalue-1)){
    klock_open(status, &usr_sem_table_lock);
    return -3;
  }
  sem->value++;
  semaphore_V(sem->ksem);
  klock_open(status, &usr_sem_table_lock);
  return 0;
}
