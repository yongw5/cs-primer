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
//#include <sys/types.h>
//#include <sys/stat.h>
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
  int fd;
  struct shmstruct *ptr;
  
  if(argc != 3) {
	printf("%s <shmname> <semname>\n", argv[0]);
	exit(1);
  }

  shm_unlink(argv[1]); /* ok if this fail */
  if((fd = shm_open(argv[1], O_RDWR | O_CREAT | O_EXCL, FILE_MODE)) == -1)
	error_handler(sem_open error);
  if(ftruncate(fd, sizeof(struct shmstruct)) == -1)
	error_handler(ftruncate error);
  if((ptr = mmap(NULL, sizeof(struct shmstruct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
	error_handler(mmap error);
  close(fd);

  sem_unlink(argv[2]); /*ok if this failed*/
  if((mutex = sem_open(argv[2], O_CREAT | O_EXCL, FILE_MODE, 1)) == SEM_FAILED)
	error_handler(sem_open error);
  sem_close(mutex);
  return 0;
}
