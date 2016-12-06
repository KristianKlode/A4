/// Kernel support for userland processes.

#ifndef KUDOS_PROC_PROCESS_H
#define KUDOS_PROC_PROCESS_H

#include "lib/types.h"
#include "vm/memory.h"

#define PROCESS_PTABLE_FULL  (-1)
#define PROCESS_ILLEGAL_JOIN (-2)

#define PROCESS_MAX_FILELENGTH (256)
#define PROCESS_MAX_PROCESSES  (128)
#define PROCESS_MAX_FILES      (10)

typedef int pid_t;

typedef struct {
  pid_t pid;
  char path[256];
  enum process_state state;
  uint64_t retval;
} pcb_t;

/// Initialize process table.
/// Should be called during boot.
void process_init();

int process_read(int filehandle, void *buffer, int length);

int process_write(int filehandle, const void *buffer, int length);

/// Load and run the executable as a new process in a new thread.
/// Arguments: Path to the executable and arguments.
/// Flags are documented below.
/// Returns the process ID of the new process.
pid_t process_spawn(char const* executable, int flags);

/// Stop the current process and the kernel thread in which it runs
/// Argument: return value
void process_exit(int retval);
/// Wait for the given process to terminate, return its return value,
/// and mark the process-table entry as free
int process_join(process_id_t pid);

#endif // KUDOS_PROC_PROCESS_H
