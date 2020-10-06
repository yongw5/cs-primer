/**
 * one producer and one consumer based on memory-based semaphore
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
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)

int nitems; /* read-only by producer and consumer */
struct {
  int buf[NBUFF];
  sem_t mutex, nempty, nstored;
} shared;
void *producer(void *), *consumer(void *);

int main(int argc, char **argv) {
  pthread_t tid_producer, tid_consumer;
  if(argc != 2) {
	printf("usage: %s <#items>\n", argv[0]);
	exit(1);
  }
  nitems = atoi(argv[1]);

  /* create three semaphores */
  if(sem_init(&shared.mutex, 0, 1) == -1)
	error_handler(sem_init error);
  if(sem_init(&shared.nempty, 0, NBUFF) == -1)
	error_handler(sem_init error);
  if(sem_init(&shared.nstored, 0, 0) == -1)
	error_handler(sem_init error);

  /* create one producer thread and one consumer thread */
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
  for(i = 0; i < nitems; ++i) {
	sem_wait(&shared.nempty); /* wait for at least 1 empty slot */
	sem_wait(&shared.mutex);
	shared.buf[i % NBUFF] = i; /* store i into circular buffer */
	sem_post(&shared.mutex);
	sem_post(&shared.nstored); /* 1 more stored item */
  }
  return NULL;
}

void *consumer(void *arg) {
  int i;
  for(i = 0; i < nitems; ++i) {
	sem_wait(&shared.nstored); /* wait for at least 1 stored item */
	sem_wait(&shared.mutex);
	if(shared.buf[i % NBUFF] != i)
	  printf("buf[%d] = %d\n", i, shared.buf[i % NBUFF]);
	sem_post(&shared.mutex);
	sem_post(&shared.nempty);
  }
  return NULL;
}
