#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#define MAXNITEMS   1000000
#define MAXNTHREADS 100
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(1)
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

int nitems;  /* read-only by producer and consumer */
struct {
  pthread_mutex_t mutex;
  int buf[MAXNITEMS];
  int nput;
  int nval;
} shared = { PTHREAD_MUTEX_INITIALIZER };
void *producer(void*);
void *consumer(void*);

int main(int argc, char **argv) {
  int i, nthreads, count[MAXNTHREADS];
  pthread_t tid_producers[MAXNTHREADS], tid_consumer;
  
  if(argc != 3) {
	printf("usage: %s <#items> <#threads>\n", argv[0]);
	exit(1);
  }
  nitems = min(atoi(argv[1]), MAXNITEMS);
  nthreads = min(atoi(argv[2]), MAXNTHREADS);
  pthread_setconcurrency(nthreads);

  for(i = 0; i < nthreads; ++i) { /*start all the producer threads */ 
    count[i] = 0;
	pthread_create(&tid_producers[i], NULL, producer, &count[i]);
  }

  for(i = 0; i < nthreads; ++i) { /* wait for all the producer threads */ 
    pthread_join(tid_producers[i], NULL);
	printf("count[%d] = %d\n", i, count[i]);
  }

  /* start, then wait for consumer thread */
  pthread_create(&tid_consumer, NULL, consumer, NULL);
  pthread_join(tid_consumer, NULL);
  return 0;
}

void *producer(void *arg) {
  while(1) {
	pthread_mutex_lock(&shared.mutex);
	if(shared.nput >= nitems) {
	  pthread_mutex_unlock(&shared.mutex);
	  return NULL;
	}
	shared.buf[shared.nput] = shared.nval;
	++shared.nput;
	++shared.nval;
	pthread_mutex_unlock(&shared.mutex);
	*((int*)arg) += 1;
  }
  return NULL;
}

void *consumer(void *arg) {
  int i;
  for(i = 0; i < nitems; ++i) {
	if(shared.buf[i] != i)
	  printf("buf[%d] = %d\n", i, shared.buf[i]);
  }
  return NULL;
}
