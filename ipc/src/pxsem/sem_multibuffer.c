/**
 * file copying divided into two threads using two buffers
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)
#define NBUFF    8
#define BUFFSIZE 1024
struct {                   /* data shared by producer and consumer */
  struct {
	char data[BUFFSIZE];   /* a buffer */
	ssize_t n;             /* count of #bytes in the buffer */
  } buff[NBUFF];
  sem_t mutex, nempty, nstored;
} shared;

int fd;                    /* input file to copy to stdout */
void *producer(void *), *consumer(void *);


int main(int argc, char **argv) {
  pthread_t tid_producer, tid_consumer;
  if(argc != 2) {
	printf("usage: %s <pathname>\n", argv[0]);
	exit(1);
  }
  if((fd = open(argv[1], O_RDONLY)) == -1)
	error_handler(open error);

  /* create three semaphores */
  if(sem_init(&shared.mutex, 0, 1) == -1)
	error_handler(sem_init error);
  if(sem_init(&shared.nempty, 0, NBUFF) == -1)
	error_handler(sem_init error);
  if(sem_init(&shared.nstored, 0, 0) == -1)
	error_handler(sem_init error);

  /* one producer thread, one consumer thread*/
  pthread_setconcurrency(2); 
  if(pthread_create(&tid_producer, NULL, producer, NULL) != 0)
	error_handler(pthread_create error);
  if(pthread_create(&tid_consumer, NULL, consumer, NULL) != 0)
	error_handler(pthread_create error);
  
  /* wait for the two threads */
  if(pthread_join(tid_producer, NULL) != 0)
	error_handler(pthread_join error);
  if(pthread_join(tid_consumer, NULL) != 0)
	error_handler(pthread_join error);

  if(sem_destroy(&shared.mutex) == -1)
	error_handler(sem_destroy error);
  if(sem_destroy(&shared.nempty) == -1)
	error_handler(sem_destroy error);
  if(sem_destroy(&shared.nstored) == -1)
	error_handler(sem_destroy error);
  
  return 0;
}


void *producer(void *arg) {
  int i = 0;
  while(1) {
	sem_wait(&shared.nempty);
	sem_wait(&shared.mutex);
	/* critical region */
	sem_post(&shared.mutex);
   
	if((shared.buff[i].n = read(fd, shared.buff[i].data, BUFFSIZE)) == -1)
	  error_handler(read error);

	if(shared.buff[i].n == 0) {
	  sem_post(&shared.nstored); /* 1 more stored item */
	  return NULL;
	}
	if(++i >= NBUFF)
	  i = 0;
	sem_post(&shared.nstored);
  }
  return NULL;
}

void *consumer(void *arg) {
  int i = 0;
  while(1) {
	sem_wait(&shared.nstored);
	sem_wait(&shared.mutex);
	/* critical region */
	sem_post(&shared.mutex);
    
	if(shared.buff[i].n == 0)
	  return NULL;
	write(STDOUT_FILENO, shared.buff[i].data, shared.buff[i].n);
	if(++i >= NBUFF)
	  i = 0;
	sem_post(&shared.nempty);
  }
  return NULL;
}
