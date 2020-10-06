/*
 * fork example
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int gval = 10;
int main(int argc, char **argv) {
  pid_t pid;
  int lval = 20;
  gval++, lval += 5;
  printf("origin val: gval = %d, lval = %d\n", gval, lval);
  pid = fork();
  if(pid == 0) // child process
    gval += 2, lval += 2;
  else
    gval -= 2, lval -= 2;
  if(pid == 0)
    printf("child process: gval = %d, lval = %d\n", gval, lval);
  else
    printf("parent process: gval = %d, lval = %d\n", gval, lval);
  return 0;
}
