/**
 * a server is started that created a shared memory object in which 
 * message are placed by client processes. our server just prints 
 * these messages, although this coudl be generalized to do things
 * similaryly to the syslog daemon
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/errno.h>

#define MSG_SIZE  256  /* max #bytes per message */
#define NMSG       16  /* max #messages */
#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)

struct shmstruct {       /* struct stored in shared memory */
  sem_t mutex;           /* three posix memory-based semaphore */
  sem_t nempty;
  sem_t nstroed;
  int   nput;            /* index into msgoff[] for next put */
  long  noverflow;       /* #overflow by senders */
  sem_t noverflowmutex;  /* mutex for noverflow counter */
  long  msgoff[NMSG];    /* offset in shared memory of each message */
  char  msgdata[NMSG * MSG_SIZE]; /* the actual message */
};

int main(int argc, char **argv) {
  int fd, index, lastnoverflow, temp;
  long offset;
  struct shmstruct *ptr;

  if(argc != 2) {
	printf("usage: %s <name>\n", argv[0]);
	exit(1);
  }

  /* create shm, set its size, map it, close descriptor */
  shm_unlink(argv[1]);
  if((fd = shm_open(argv[1], O_RDWR | O_CREAT | O_EXCL, FILE_MODE)) == -1)
	error_handler(shm_open error);
  if((ptr = mmap(NULL, sizeof(struct shmstruct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
	error_handler(mmap error);
  if(ftruncate(fd, sizeof(struct shmstruct)) == -1)
	error_handler(ftruncate error);
  close(fd);

  /* initialize the array of offset */
  for(index = 0; index < NMSG; ++index)
	ptr->msgoff[index] = index * MSG_SIZE;

  /* initialize the semaphore in shared memory */
  if(sem_init(&ptr->mutex, 1, 1) == -1)
	error_handler(sem_init error);
  if(sem_init(&ptr->nempty, 1, NMSG) == -1)
	error_handler(sem_init error);
  if(sem_init(&ptr->nstroed, 1, 0) == -1)
	error_handler(sem_init error);
  if(sem_init(&ptr->noverflowmutex, 1, 1) == -1)
	error_handler(sem_init error);

  /* this program is the consumer */
  index = 0;
  lastnoverflow = 0;
  while(1) {
	sem_wait(&ptr->nstroed);
	sem_wait(&ptr->mutex);
	offset = ptr->msgoff[index];
	printf("index = %d: %s\n", index, &ptr->msgdata[offset]);
	index = (index + 1) % NMSG;
	sem_post(&ptr->mutex);
	sem_post(&ptr->nempty);

	sem_wait(&ptr->noverflowmutex);
	temp = ptr->noverflow;
	sem_post(&ptr->noverflowmutex);
	if(temp != lastnoverflow) {
	  printf("noverflow = %d\n", temp);
	  lastnoverflow = temp;	  
	}
  }
  return 0;
}

