#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <error.h>

#define error_handler(msg) \
  do { \
	printf("%s:%d %s\n", __FILE__, __LINE__, #msg); \
	exit(1); \
  }while(0)

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
pthread_t tid1, tid2;
void *thread1(void *);
void *thread2(void *);

int main(int argc, char **argv) {
  void *status;
  pthread_setconcurrency(2);
  if(pthread_create(&tid1, NULL, thread1, NULL))
	error_handler(pthread_create error);
  sleep(1); /* let thread1() get the lock */
  if(pthread_create(&tid2, NULL, thread2, NULL))
	error_handler(pthread_create error);

  if(pthread_join(tid2, &status))
	error_handler(pthread_join error);
  if(status != PTHREAD_CANCELED)
	printf("thread2 status = %p\n", status);
  
  if(pthread_join(tid1, &status))
	error_handler(pthread_join error);
  if(status != NULL)
	printf("thread1 status = %p\n", status);

  if(pthread_rwlock_destroy(&rwlock))
	error_handler(pthread_rwlock_destroy error);
  return 0;
}

void *thread1(void *arg) {
  if(pthread_rwlock_rdlock(&rwlock))
	error_handler(pthread_rwlock_rdlock error);
  printf("thread1 got a read lock\n");
  sleep(3); /* let thread2 block iin thread_rwlock_wrlock */
  pthread_cancel(tid2);
  printf("thread2 is cancled\n");
  sleep(3);
  if(pthread_rwlock_unlock(&rwlock))
	error_handler(pthread_rwlock_unlock error);
  return NULL;
}

void *thread2(void *arg) {
  printf("thread2 trying to obtain a write lock\n");
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  if(pthread_rwlock_wrlock(&rwlock))
	error_handler(pthread_rwlock_wrlock error);
  printf("thread2 got a write lock\n"); /* should not get here */
  sleep(1);
  if(pthread_rwlock_unlock(&rwlock))
	error_handler(pthread_rwlock_unlock error);
  return NULL;
}
