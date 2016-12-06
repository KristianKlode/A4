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
    usr_sem_table_lock[i].value = -1;
    usr_sem_table_lock[i].maxvalue = -1;
    usr_sem_table_lock[i].name = "";
    usr_sem_table_lock[i].creator = -1;
  }
}




usr_sem_t* usr_sem_open(const char* name, int value){
    for (i = 0; i < USR_SEM_MAX_SEMS; ++i) {
      if(usr_sem_table[i].name == ""){
        klock_status_t status = klock_lock(&usr_sem_table_lock);
        usr_sem_table[i].value = value;
        usr_sem_table[i].maxvalue = value;
        usr_sem_table[i].name = name;
        usr_sem_table[i].creator = thread_get_current_thread();
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
  sem->value = -1;
  sem->maxvalue = -1;
  sem->name = "";
  sem->creator = -1;
  klock_open(status, &usr_sem_table_lock);
  return &usr_sem_table[i];
}
