#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#define error_handler(msg) \
  do { \
    printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
  }while(0)

mqd_t mqd;
struct mq_attr attr;
struct sigevent sigev;
static void notify_thread(union sigval);

int main(int argc, char **argv) {
  if(argc != 2) {
	printf("usage: %s <mqname>\n", argv[0]);
	exit(1);
  }
  if((mqd = mq_open(argv[1], O_RDONLY | O_NONBLOCK)) == -1)
	error_handler(mq_open error);
  mq_getattr(mqd, &attr);

  sigev.sigev_notify = SIGEV_THREAD;
  sigev.sigev_value.sival_ptr = NULL;
  sigev.sigev_notify_function = notify_thread;
  sigev.sigev_notify_attributes = NULL;
  if(mq_notify(mqd, &sigev) == -1)
	error_handler(mq_notify error);

  int i;
  for(i = 0; i < 3; ++i)
	pause();
  return 0;
}

static void notify_thread(union sigval arg) {
  ssize_t n;
  void *buf;
  unsigned int prio;

  printf("notify_thread started\n");
  if((buf = malloc(attr.mq_msgsize)) == NULL)
	error_handler(malloc error);
  mq_notify(mqd, &sigev);
  while((n = mq_receive(mqd, buf, attr.mq_msgsize, &prio)) >= 0)
	printf("read %ld bytes, priority %d\n", (long)n, prio);
  if(errno != EAGAIN) {
	free(buf);
	error_handler(mq_receive error);
  }
  free(buf);
  pthread_exit(NULL);
}
