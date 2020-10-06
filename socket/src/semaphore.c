/*
 * 信号量量进程同步
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

void *read(void *arg);
void *accu(void *arg);
static sem_t sem_one, sem_two;
static int num;

int main(int argc, char **argv) {
  pthread_t t1_id, t2_id;
  sem_init(&sem_one, 0, 0);
  sem_init(&sem_two, 0, 1);

  pthread_create(&t1_id, NULL, read, NULL);
  pthread_create(&t2_id, NULL, accu, NULL);

  pthread_join(t1_id, NULL);
  pthread_join(t2_id, NULL);

  sem_destroy(&sem_one);
  sem_destroy(&sem_two);
  return 0;
}

void *read(void *arg) {
  int i;
  for(i = 0; i < 5; ++i) {
	fputs("input num: ", stdout);
	sem_wait(&sem_two);
	scanf("%d", &num);
	sem_post(&sem_one);
  }
  return NULL;
}

void *accu(void *arg) {
  int sum = 0, i;
  for(i = 0; i < 5; ++i) {
	sem_wait(&sem_one);
	sum += num;
	sem_post(&sem_two);
  }
  printf("result: %d\n", sum);
  return NULL;
}
