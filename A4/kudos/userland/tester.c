#include "lib.h"

int main() {
  printf("Hej og farvel");
  syscall_exit(42);
  return 43;
}
