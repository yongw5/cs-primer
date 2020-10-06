#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  } while(0)

volatile sig_atomic_t mqflag; /* set non zero by signal handler */
static void sig_usr1(int);

int main(int argc, char **argv) {
  mqd_t mqd;
  void *buf;
  ssize_t n;
  sigset_t zeromask, newmask, oldmask;
  struct mq_attr attr;
  struct sigevent sigev;
  if(argc != 2) {
	printf("usage: %s <mqname>\n", argv[0]);
	exit(1);
  }
  if((mqd = mq_open(argv[1], O_RDONLY)) == -1)
	error_handler(mq_open() error);
  if(mq_getattr(mqd, &attr) == -1)
	error_handler(mq_getattr() error);
  if((buf = malloc(attr.mq_msgsize)) == NULL)
	error_handler(malloc() error);
  
  sigemptyset(&zeromask);
  sigemptyset(&newmask);
  sigemptyset(&oldmask);
  sigaddset(&newmask, SIGUSR1);

  signal(SIGUSR1, sig_usr1);
  sigev.sigev_notify = SIGEV_SIGNAL;
  sigev.sigev_signo = SIGUSR1;
  if(mq_notify(mqd, &sigev) == -1)
	error_handler(mq_notify() error);
  int i;
  for(i = 0; i < 3; ++i) {
	sigprocmask(SIG_BLOCK, &newmask, &oldmask);  /* block SIGUSR1 */
	while(!mqflag) 
	  sigsuspend(&zeromask);
	mqflag = 0;
	if(mq_notify(mqd, &sigev) == -1) {
	  free(buf);
	  error_handler(mq_notify() error);
	}
	if((n = mq_receive(mqd, buf, attr.mq_msgsize, NULL)) == -1) {
	  free(buf);
	  error_handler(mq_receive() error);
	}
	printf("read %ld bytes\n", (long)n);
	sigprocmask(SIG_UNBLOCK, &newmask, NULL);
  }
  free(buf);
  return 0;
}

static void sig_usr1(int signo) {
  mqflag = 1;
}
