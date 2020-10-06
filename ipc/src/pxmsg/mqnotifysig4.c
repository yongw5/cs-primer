#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <mqueue.h>
#include <signal.h>
#include <errno.h>

#define error_handler(msg) \
  do { \
    printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)

int main(int argc, char **argv) {
  int signo;
  mqd_t mqd;
  void *buf;
  ssize_t n;
  sigset_t newmask;
  struct mq_attr attr;
  struct sigevent sigev;

  if(argc != 2) {
	printf("usage: %s <mqname>\n", argv[0]);
	exit(1);
  }

  if((mqd = mq_open(argv[1], O_RDONLY | O_NONBLOCK)) == -1)
	error_handler(mq_open error);
  mq_getattr(mqd, &attr);
  if((buf = malloc(attr.mq_msgsize)) == NULL)
	error_handler(malloc error);

  sigemptyset(&newmask);
  sigaddset(&newmask, SIGUSR1);
  sigprocmask(SIG_BLOCK, &newmask, NULL); /* block SIGUSR1 */

  sigev.sigev_notify = SIGEV_SIGNAL;
  sigev.sigev_signo = SIGUSR1;
  if(mq_notify(mqd, &sigev) == -1)
	error_handler(mq_notify error);

  int i;
  for(i = 0; i < 3; ++i) {
	sigwait(&newmask, &signo);
	if(signo == SIGUSR1) {
	  if(mq_notify(mqd, &sigev) == -1) {
		free(buf);
		error_handler(mq_notify error);
	  }
	  while((n = mq_receive(mqd, buf, attr.mq_msgsize, NULL)) >= 0)
		printf("read %ld bytes\n", (long)n);
	  if(errno != EAGAIN)
		error_handler(mq_receive error);
	}
  }
  free(buf);
  return 0;
}
