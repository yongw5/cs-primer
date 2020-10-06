/**
 * multiple processes increment a counter that is stored in shared memory.
 * we store the counter in shared memory and use a named semaphore for
 * synchronization, but we no longer need a parent-child relationship
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/errno.h>

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)

struct shmstruct { /* struct stored in shared memory */
  int count;
};
sem_t *mutex;      /* pointer to named semaphore */

int main(int argc, char **argv) {
  int fd, i, nloop;
  pid_t pid;
  struct shmstruct *ptr;
  
  if(argc != 4) {
	printf("%s <shmname> <semname> <#loops>\n", argv[0]);
	exit(1);
  }
  nloop = atoi(argv[3]);

  if((fd = shm_open(argv[1], O_RDWR, FILE_MODE)) == -1)
	error_handler(sem_open error);
  if((ptr = mmap(NULL, sizeof(struct shmstruct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
	error_handler(mmap error);
  close(fd);

  if((mutex = sem_open(argv[2], 0)) == SEM_FAILED)
	error_handler(sem_open error);
  pid = getpid();
  for(i = 0; i < nloop; ++i) {
	sem_wait(mutex);
	printf("pid %ld: %d\n", (long)pid, ptr->count++);
	sem_post(mutex);
  } 
  return 0;
}
