/**
 * one producer and one consumer based on named semaphore
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
#define SEM_MUTEX   "/prodcons1.mutex"
#define SEM_NEMPTY  "/prodcons1.nempty"
#define SEM_NSTORED "/prodcons1.nstored"
#define FILE_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)

int nitems;
struct {
  int buf[NBUFF];
  sem_t *mutex, *nempty, *nstored;
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
  if((shared.mutex = sem_open(SEM_MUTEX, O_CREAT | O_EXCL, FILE_MODE, 1)) == SEM_FAILED)
	error_handler(sem_open error);
  if((shared.nempty = sem_open(SEM_NEMPTY, O_CREAT | O_EXCL, FILE_MODE, NBUFF)) == SEM_FAILED)
	error_handler(sem_open error);
  if((shared.nstored = sem_open(SEM_NSTORED, O_CREAT | O_EXCL, FILE_MODE, 0)) == SEM_FAILED)
	error_handler(sem_open error);

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
  if(sem_unlink(SEM_MUTEX) == -1)
	error_handler(sem_unlink error);
  if(sem_unlink(SEM_NEMPTY) == -1)
	error_handler(sem_unlink error);
  if(sem_unlink(SEM_NSTORED) == -1)
	error_handler(sem_unlink error);
  return 0;
}

void *producer(void *arg) {
  int i;
  for(i = 0; i < nitems; ++i) {
	sem_wait(shared.nempty); /* wait for at least 1 empty slot */
	sem_wait(shared.mutex);
	shared.buf[i % NBUFF] = i; /* store i into circular buffer */
	sem_post(shared.mutex);
	sem_post(shared.nstored); /* 1 more stored item */
  }
  return NULL;
}

void *consumer(void *arg) {
  int i;
  for(i = 0; i < nitems; ++i) {
	sem_wait(shared.nstored); /* wait for at least 1 stored item */
	sem_wait(shared.mutex);
	if(shared.buf[i % NBUFF] != i)
	  printf("buf[%d] = %d\n", i, shared.buf[i % NBUFF]);
	sem_post(shared.mutex);
	sem_post(shared.nempty);
  }
  return NULL;
}
