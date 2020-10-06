#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <errno.h>

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define SEM_NAME "/incr.sem"
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0);

int main(int argc, char **argv) {
  int fd, i, nloop, zero = 0;
  int *ptr;
  sem_t *mutex;

  if(argc != 3) {
	printf("usage: %s <pathname> <#nloop>\n", argv[0]);
	exit(1);
  }
  nloop = atoi(argv[2]);

  /* open file, initialize to 0, map into memory */
  if((fd = open(argv[1], O_RDWR | O_CREAT, FILE_MODE)) == -1)
	error_handler(open error);
  if(write(fd, &zero, sizeof(int)) == -1)
	error_handler(write error);
  if((ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
	error_handler(mmap error);
  close(fd);

  /* create, initialize, and unlink semaphore */
  if((mutex = sem_open(SEM_NAME, O_CREAT | O_EXCL, FILE_MODE, 1)) == SEM_FAILED)
	error_handler(sem_open error);
  if(sem_unlink(SEM_NAME) == -1)
	error_handler(sem_unlink error);

  setbuf(stdout, NULL); /* stdout is unbuffered */
  if(fork() == 0) {
	for(i = 0; i < nloop; ++i) {
	  sem_wait(mutex);
	  printf("child: %d\n", (*ptr)++);
	  sem_post(mutex);
	}
	return 0;
  }

  for(i = 0; i < nloop; ++i) {
	sem_wait(mutex);
	printf("parent: %d\n", (*ptr)++);
	sem_post(mutex);
  }
  return 0;

}
