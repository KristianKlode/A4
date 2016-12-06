#include "lib.h"

int main(void) {
  sem_t* sem = syscall_sem_open("sem0", 4);
  printf("Hejsa!");
  if (syscall_sem_p(sem) != 0){
    printf("fail sem");
    return -3;
  }
  if (syscall_sem_p(sem) != 0){
    printf("fail sem");
    return -3;
  }
  if (syscall_sem_p(sem) != 0){
    printf("fail sem");
    return -3;
  }
  if (syscall_sem_p(sem) != 0){
    printf("fail sem");
    return -3;
  }
  if (syscall_sem_p(sem) != -3){
    printf("fail sem");
    return -3;
  }
    printf("success sem");
    return 0;
}
