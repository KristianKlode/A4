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

#include "drivers/device.h"     // device_*
#include "drivers/gcd.h"        // gcd_*
#include "kernel/assert.h"      // KERNEL_ASSERT
#include "proc/syscall.h"       // FD_*, IO_*


usr_sem_t process_table[USR_SEM_MAX_SEMS];
klock_t usr_sem_table_lock;

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
      for (i = 0; i < USR_SEM_MAX_SEMS; ++i) {
        if(usr_sem_table[i].name == ""){
          return &usr_sem_table[i];
        }
      return NULL
      }
    }
    for (i = 0; i < USR_SEM_MAX_SEMS; ++i) {
      if(usr_sem_table[i].name == ""){
        klock_status_t status = klock_lock(&usr_sem_table_lock);
        usr_sem_table[i].value = value;
        usr_sem_table[i].maxvalue = value;
        usr_sem_table[i].name = name;
        usr_sem_table[i].ksem = SYSCALL_SEM_OPEN(value);
        klock_open(status, &usr_sem_table_lock);
        return &usr_sem_table[i];
      }
    }
  }
}

int usr_sem_close(usr_sem_t* sem){
  if (value != maxvalue){
    return(-1)
  }
  klock_init(&usr_sem_table[i].klock);
  klock_status_t status = klock_lock(&usr_sem_table_lock);
  SYSCALL_SEM_CLOSE(sem->ksem);
  sem->value = -1;
  sem->maxvalue = -1;
  sem->name = "";
  sem->ksem = -1;
  klock_open(status, &usr_sem_table_lock);
  return 0;
}

int usr_sem_p(usr_sem_t* sem){
  if(sem->value<1){
    return -2;
  }
  sem->value = sem->value -1;
  SYSCALL_SEM_P(sem->ksem);
  return 0;
}

int usr_sem_v(usr_sem_t* sem){
  if(sem->value > (sem->maxvalue-1)){
    return -3;
  }
  sem->value = sem->value +1;
  SYSCALL_SEM_V(sem->ksem);
  return 0;
}


