#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>

#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(1);

int main(int argc, char **argv) {
  sem_t *sem;
  int val;
  if(argc != 2) {
	printf("usage: %s <name>\n", argv[0]);
	exit(1);
  }
  if((sem = sem_open(argv[1], 0)) == SEM_FAILED)
	error_handler(sem_open error);
  if(sem_wait(sem) == -1)
	error_handler(sem_wait error);
  if(sem_getvalue(sem, &val) == -1)
	error_handler(sem_getvalue error);
  printf("pid %ld has semaphore, value = %d\n", (long)getpid(), val);

  pause();
  return 0;
}
