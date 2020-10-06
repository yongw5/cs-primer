#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <errno.h>

#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)

int main(int argc, char **argv) {
  int i, nloop, fd;
  int *ptr;
  sem_t mutex;

  if(argc != 2) {
	printf("usage: %s <#nloop>\n", argv[0]);
	exit(1);
  }

  nloop = atoi(argv[1]);
  
  /* open /dev/zero, map into memory */
  if((fd = open("/dev/zero", O_RDWR)) == -1)
	error_handler(open error);

  if((ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
	error_handler(mmap error);
  if(sem_init(&mutex, 1, 1) == -1)
	error_handler(sem_init error);

  setbuf(stdout, NULL);
  if(fork() == 0) {
	for(i= 0; i < nloop; ++i) {
	  sem_wait(&mutex);
	  printf("child: %d\n", (*ptr)++);
	  sem_post(&mutex);
	}
	return 0;
  }

  for(i = 0; i < nloop; ++i) {
	sem_wait(&mutex);
	printf("parent: %d\n", (*ptr)++);
	sem_post(&mutex);
  }
  sem_destroy(&mutex);
  return 0;
}
