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

extern void process_set_pagetable(pagetable_t*);

pcb_t process_table[PROCESS_MAX_PROCESSES];
klock_t process_table_lock;

void process_reset(const pid_t pid) {
  pcb_t *process = &process_table[pid];

  process->pid = -1;
}

/// Initialize process table and locks.
void process_init() {
  int i;
  klock_init(&process_table_lock);
  for (i = 0; i < PROCESS_MAX_PROCESSES; ++i) {
    process_reset(i);
  }
}

/// Find a free slot in the process table. Returns PROCESS_MAX_PROCESSES if
/// the table is full.
pid_t alloc_process_id() {
  int i;
  klock_status_t st;

  st = klock_lock(&process_table_lock);
  for (i = 0; i < PROCESS_MAX_PROCESSES; ++i) {
    if (process_table[i].pid == -1 and process_table[i].state == PROCESS_FREE) {
      memoryset(&process_table[i], 0, sizeof(pcb_t));
      process_table[i].pid = i;
      process_table[i].state = PROCESS_RUNNING;
      break;
    }
  }

  klock_open(st, &process_table_lock);
  return i;
}

/* Return non-zero on error. */
int setup_new_process(TID_t thread,
                      const char *path,
                      int flags,
                      virtaddr_t *entry_point, virtaddr_t *stack_top)
{
  pagetable_t *pagetable;
  elf_info_t elf;
  openfile_t file;
  uintptr_t phys_page;
  virtaddr_t virt_page;
  int i, res;
  thread_table_t *thread_entry = thread_get_thread_entry(thread);

  flags = flags;

  file = vfs_open((char *)path);

  /* Make sure the file existed and was a valid ELF file */
  if (file < 0) {
    return -1;
  }

  res = elf_parse_header(&elf, file);
  if (res < 0) {
    return -1;
  }

  /* Trivial and naive sanity check for entry point: */
  if (elf.entry_point <= VMM_KERNEL_SPACE) {
    return -1;
  }

  *entry_point = elf.entry_point;


  pagetable = vm_create_pagetable(thread);
  process_set_pagetable(pagetable);

  thread_entry->pagetable = pagetable;

  /* Allocate and map stack */
  for(i = 0; i < CONFIG_USERLAND_STACK_SIZE; i++) {
    phys_page = physmem_allocblock();
    KERNEL_ASSERT(phys_page != 0);
    virt_page = (USERLAND_STACK_TOP & PAGE_SIZE_MASK) - i*PAGE_SIZE;
    vm_map(pagetable, phys_page,
           virt_page, PAGE_USER | PAGE_WRITE);
    /* Zero the page */
    memoryset((void*)virt_page, 0, PAGE_SIZE);
  }

  /* Allocate and map pages for the ELF segments. We assume that
     the segments begin at a page boundary. (The linker script
     in the userland directory helps users get this right.) */
  for(i = 0; i < (int)elf.ro_pages; i++) {
    int left_to_read = elf.ro_size - i*PAGE_SIZE;
    phys_page = physmem_allocblock();
    KERNEL_ASSERT(phys_page != 0);
    virt_page = elf.ro_vaddr + i*PAGE_SIZE;
    vm_map(pagetable, phys_page,
           virt_page, PAGE_USER | PAGE_WRITE);
    /* Zero the page */
    memoryset((void*)virt_page, 0, PAGE_SIZE);
    /* Fill the page from ro segment */
    if (left_to_read > 0) {
      KERNEL_ASSERT(vfs_seek(file, elf.ro_location + i*PAGE_SIZE) == VFS_OK);
      KERNEL_ASSERT(vfs_read(file, (char*)virt_page,
                             MIN(PAGE_SIZE, left_to_read))
                    == (int) MIN(PAGE_SIZE, left_to_read));
    }
    //Make the page read only
    vm_map(pagetable, phys_page,
            virt_page, PAGE_USER);
  }

  for(i = 0; i < (int)elf.rw_pages; i++) {
    int left_to_read = elf.rw_size - i*PAGE_SIZE;
    phys_page = physmem_allocblock();
    KERNEL_ASSERT(phys_page != 0);
    virt_page = elf.rw_vaddr + i*PAGE_SIZE;
    vm_map(pagetable, phys_page,
           virt_page, PAGE_USER | PAGE_WRITE);
    /* Zero the page */
    memoryset((void*)virt_page, 0, PAGE_SIZE);
    /* Fill the page from rw segment */
    if (left_to_read > 0) {
      KERNEL_ASSERT(vfs_seek(file, elf.rw_location + i*PAGE_SIZE) == VFS_OK);
      KERNEL_ASSERT(vfs_read(file, (void*)virt_page,
                             MIN(PAGE_SIZE, left_to_read))
                    == (int) MIN(PAGE_SIZE, left_to_read));
    }
  }

  /* Done with the file. */
  vfs_close(file);

  *stack_top = USERLAND_STACK_TOP;

  return 0;
}

void process_start(uint64_t arg) {
  int pid;
  context_t user_context;
  virtaddr_t entry_point;
  virtaddr_t stack_top;

  pid = (int)arg;

  if (setup_new_process(
    thread_get_current_thread(),
    process_table[pid].path,
    0,
    &entry_point,
    &stack_top) != 0) {
    KERNEL_PANIC("setup_new_process failed");
  }

  memoryset(&user_context, 0, sizeof(user_context));

  _context_set_ip(&user_context, entry_point);
  _context_set_sp(&user_context, stack_top);

  thread_goto_userland(&user_context);
}

pid_t process_spawn(const char *executable, int flags)
{
  TID_t tid;
  pid_t pid;

  pid = alloc_process_id();
  if (pid == PROCESS_MAX_PROCESSES) {
    return PROCESS_PTABLE_FULL;
  }

  tid = thread_create((void (*)(uint64_t))(&process_start), pid);
  thread_get_thread_entry(tid)->pid = pid;

  stringcopy(&process_table[pid].path, executable, strlen(executable) + 1);

  thread_run(tid);

  return pid;
}

pid_t process_get_current_process(void) {
  return thread_get_current_thread_entry()->pid;
}

pcb_t *process_get_current_process_entry(void) {
  return &process_table[process_get_current_process()];
}

static int tty_read(void *buffer, int length) {
  device_t *dev;
  gcd_t *gcd;

  dev = device_get(TYPECODE_TTY, 0);
  if (dev == NULL) return IO_TTY_UNAVAILABLE;
  gcd = (gcd_t *)dev->generic_device;
  if (gcd == NULL) return IO_TTY_UNAVAILABLE;

  return gcd->read(gcd, buffer, length);
}

static int tty_write(const void *buffer, int length) {
  device_t *dev;
  gcd_t *gcd;

  dev = device_get(TYPECODE_TTY, 0);
  if (dev == NULL) return IO_TTY_UNAVAILABLE;
  gcd = (gcd_t *)dev->generic_device;
  if (gcd == NULL) return IO_TTY_UNAVAILABLE;

  return gcd->write(gcd, buffer, length);
}

int process_read(int filehandle, void *buffer, int length) {
  int retval = 0;

  // We might want to check that we're not reading from kernel memory..

  if (length < 0) {
    retval = IO_NEGATIVE_LENGTH;
  } else if (filehandle == FD_STDIN) {
    retval = tty_read(buffer, length);
  } else if (filehandle == FD_STDOUT
            || filehandle == FD_STDERR) {
    retval = IO_INVALID_HANDLE;
  } else {
    retval = IO_NOT_IMPLEMENTED;
  }

  return retval;
}

int process_write(int filehandle, const void *buffer, int length) {
  int retval = 0;

  // We might want to check that we're not writing to kernel memory..

  if (length < 0) {
    retval = IO_NEGATIVE_LENGTH;
  } else if (filehandle == FD_STDIN) {
    retval = IO_INVALID_HANDLE;
  } else if (filehandle == FD_STDOUT
            || filehandle == FD_STDERR) {
    retval = tty_write(buffer, length);
  } else {
    retval = IO_NOT_IMPLEMENTED;
  }

  return retval;

/// Stop the current process and the kernel thread in which it runs
/// Argument: return value
void process_exit(int retval);{
  thread_table_t* thr;
  thr = thread_get_current_thread_entry();
  pid_t pid = process_get_current_process();
  process_table[pid].state = PROCESS_ZOMBIE;
  process_table[pid].retval = retval;
  vm_destroy_pagetable(thr->pagetable);
  thr->pagetable = NULL;
  thread_finish();
}

/// Wait for the given process to terminate, return its return value,
/// and mark the process-table entry as free
int process_join(pid_t pid){
        int retval;
        while (process_table[i].state != PROCESS_ZOMBIE){
          status = klock_lock(&process_table_lock);
          sleepq_add(thread_table[i]);
          klock_open(status, &process_table_lock);
          thread_switch();
          status = klock_lock(&process_table_lock);
        }
        retval = process_table[i].retval;
        process_table[i].state = PROCESS_FREE;
        process_table[i].pid = -1;
        klock_open(status, &process_table_lock);
        return retval
}

