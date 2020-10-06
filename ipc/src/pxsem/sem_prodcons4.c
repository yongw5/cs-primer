/**
 * multi-producer and multi-consumer based on memory-based semaphore
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>

#define NBUFF       10
#define MAXNTHREADS 100
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)
#define min(a, b) ((a) < (b) ? (a) : (b))

int nitems, nproducers, nconsumers; /* read-only by producer and consumer */
struct {
  int buf[NBUFF];
  int nput;
  int nputval;
  int nget;
  int ngetval;
  sem_t mutex, nempty, nstored;
} shared;
void *producer(void *), *consumer(void *);

int main(int argc, char **argv) {
  int i, prodcount[MAXNTHREADS], conscount[MAXNTHREADS];
  pthread_t tid_producer[MAXNTHREADS], tid_consumer[MAXNTHREADS];

  if(argc != 4) {
	printf("usage: %s <#items> <#producers> <#consumer>\n", argv[0]);
	exit(1);
  }
  nitems = atoi(argv[1]);
  nproducers = min(atoi(argv[2]), MAXNTHREADS);
  nconsumers = min(atoi(argv[3]), MAXNTHREADS);

  /* create three semaphores */
  if(sem_init(&shared.mutex, 0, 1) == -1)
	error_handler(sem_init error);
  if(sem_init(&shared.nempty, 0, NBUFF) == -1)
	error_handler(sem_init error);
  if(sem_init(&shared.nstored, 0, 0) == -1)
	error_handler(sem_init error);

  /* create all producer thread and one consumer thread */
  pthread_setconcurrency(nproducers+nconsumers);
  for(i = 0; i < nproducers; ++i) {
	prodcount[i] = 0;
	if(pthread_create(&tid_producer[i], NULL, producer, &prodcount[i]) != 0)
	  error_handler(pthread_create error);
  }
  for(i = 0; i < nconsumers; ++i) {
	conscount[i] = 0;
    if(pthread_create(&tid_consumer[i], NULL, consumer, &conscount[i]) != 0)
      error_handler(pthread_create error);
  }

  /* wait for all producers and the consumers */
  for(i = 0; i < nproducers; ++i) {
	if(pthread_join(tid_producer[i], NULL) != 0)
	  error_handler(pthread_join error);
	printf("producer count[%d] = %d\n", i, prodcount[i]);
  }
  for(i = 0; i < nconsumers; ++i) {
    if(pthread_join(tid_consumer[i], NULL) != 0)
	  error_handler(pthread_join error);
	printf("consumer count[%d] = %d\n", i, conscount[i]);
  }
  /* remove the semaphores */
  if(sem_destroy(&shared.mutex) == -1)
	error_handler(sem_destroy error);
  if(sem_destroy(&shared.nempty) == -1)
	error_handler(sem_destroy error);
  if(sem_destroy(&shared.nstored) == -1)
	error_handler(sem_destroy error);

  return 0;
}

void *producer(void *arg) {
  int i;
  while(1) { 
	sem_wait(&shared.nempty); /* wait for at least 1 empty slot */
	sem_wait(&shared.mutex);

	if(shared.nput >= nitems) {
	  sem_post(&shared.nstored); /* ##let consumers terminate */
	  sem_post(&shared.mutex);
	  sem_post(&shared.nempty);
	  return NULL;
	}

	shared.buf[shared.nput % NBUFF] = shared.nputval; /* store  into circular buffer */
	++shared.nput;
	++shared.nputval;
	sem_post(&shared.mutex);
	sem_post(&shared.nstored); /* 1 more stored item */
	*((int*)arg) += 1;
  }
  return NULL;
}

void *consumer(void *arg) {
  int i;
  while(1) {
	sem_wait(&shared.nstored); /* wait for at least 1 stored item */
	sem_wait(&shared.mutex);
	if(shared.nget >= nitems) {
	  sem_post(&shared.nstored);
	  sem_post(&shared.mutex);
	  return NULL;
	}
	i = shared.nget % NBUFF;
	if(shared.buf[i] != shared.ngetval)
	  printf("error buf[%d] = %d\n", i, shared.buf[i]);
	++shared.nget;
	++shared.ngetval;
	sem_post(&shared.mutex);
	sem_post(&shared.nempty);
	*((int*)arg) += 1;
  }
  return NULL;
}
