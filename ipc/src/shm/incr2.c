#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <errno.h>

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0);
struct shared {
  sem_t mutex;
  int count;
} shared;


int main(int argc, char **argv) {
  int fd, i, nloop;
  struct shared *ptr;

  if(argc != 3) {
	printf("usage: %s <pathname> <#nloop>\n", argv[0]);
	exit(1);
  }
  nloop = atoi(argv[2]);

  /* open file, initialize to 0, map into memory */
  if((fd = open(argv[1], O_RDWR | O_CREAT, FILE_MODE)) == -1)
	error_handler(open error);
  if(write(fd, &shared, sizeof(struct shared)) == -1)
	error_handler(write error);
  if((ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
	error_handler(mmap error);
  close(fd);

  /* create, initialize, and unlink semaphore */
  if(sem_init(&ptr->mutex, 1, 1) == -1) /* shared between processes */

  setbuf(stdout, NULL); /* stdout is unbuffered */
  if(fork() == 0) {
	for(i = 0; i < nloop; ++i) {
	  sem_wait(&ptr->mutex);
	  printf("child: %d\n", ptr->count++);
	  sem_post(&ptr->mutex);
	}
	return 0;
  }

  for(i = 0; i < nloop; ++i) {
	sem_wait(&ptr->mutex);
	printf("parent: %d\n", ptr->count++);
	sem_post(&ptr->mutex);
  }
  sem_destroy(&ptr->mutex);
  return 0;

}
