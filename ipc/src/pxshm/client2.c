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
  int fd, index, nloop, nusec;
  pid_t pid;
  char msg[MSG_SIZE];
  long offset;
  struct shmstruct *ptr;

  if(argc != 4) {
	printf("usage: %s <name> <#loops> <#usec>\n", argv[0]);
	exit(1);
  }
  nloop = atoi(argv[2]);
  nusec = atoi(argv[3]);

  /* open and map shared memory that server must create */
  if((fd = shm_open(argv[1], O_RDWR, FILE_MODE)) == -1)
	error_handler(shm_open error);
  if((ptr = mmap(NULL, sizeof(struct shmstruct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
	error_handler(mmap error);
  close(fd);

  pid = getpid();
  for(index = 0; index < nloop; ++index) {
	usleep(nusec);
	snprintf(msg, MSG_SIZE, "pid %ld: message %d", (long)pid, index);

	if(sem_trywait(&ptr->nempty) == -1) {
	  if(errno == EAGAIN) {
		sem_wait(&ptr->noverflowmutex);
		ptr->noverflow++;
		sem_post(&ptr->noverflowmutex);
		continue;
	  } else
		error_handler(sem_trywait error);
	}
	sem_wait(&ptr->mutex);
	offset = ptr->msgoff[ptr->nput];
	if(++(ptr->nput) >= NMSG)
	  ptr->nput = 0;
	sem_post(&ptr->mutex);
	strcpy(&ptr->msgdata[offset], msg);
	sem_post(&ptr->nstroed);
  }
  return 0;
}

