/*
 * 线程创建
 * 临界区
 */

#include <stdio.h>
#include <pthread.h>

void *thread_sum(void *arg);
int sum = 0;

int main(int argc, char **argv) {
  pthread_t t1_id, t2_id;
  int range1[] = {1, 5}, range2[] = {6, 10};
  pthread_create(&t1_id, NULL, thread_sum, (void*)range1);
  pthread_create(&t2_id, NULL, thread_sum, (void*)range2);
  pthread_join(t1_id, NULL);
  pthread_join(t2_id, NULL);
  printf("result: %d\n", sum);
  return 0;
}

void *thread_sum(void *arg) {
  int start = ((int*)arg)[0];
  int end = ((int*)arg)[1];

  while(start <= end) {
	sum += start;
	++start;
  }
  return NULL;
}
