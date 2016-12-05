#include "lib.h"

#define tostart "[disk]tenlines"
#define HOWMANY 10

int main(void) {
  int pids[HOWMANY];

  for (int i = 0; i < HOWMANY; i++) {
    if ((pids[i] = syscall_spawn(tostart, 0)) < 0) {
      printf("spawnn: process_spawn() numer %d failed\n", i);
      syscall_halt();
    }
  }

  return 0;
}
