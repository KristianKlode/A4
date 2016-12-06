#include "lib.h"

int main(void) {
  int new = syscall_spawn("[disk]tester", 0);
  int join = syscall_join(new);
  printf("%d\n", join);
  return 41;
}
