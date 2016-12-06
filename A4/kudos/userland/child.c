/// syscall_read/syscall_write test program
///
/// Uses library implementation of printf instead of using syscall_write
/// directly.

#include "lib.h"
#include "../kudos/proc/syscall.h"

#define LENGTH (64)

void child(void) {
  syscall_exit(43);
}
