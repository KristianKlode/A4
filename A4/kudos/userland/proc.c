/// syscall_read/syscall_write test program
///
/// Uses library implementation of printf instead of using syscall_write
/// directly.

#include "lib.h"
#include "../kudos/proc/syscall.h"

#define LENGTH (64)

int child(void) {
  return 43;
}

void main(void) {
  pid_t pid = syscall_spawn("child", 0);
  int retval = syscall_join(pid);
  syscall_exit(retval);
}
