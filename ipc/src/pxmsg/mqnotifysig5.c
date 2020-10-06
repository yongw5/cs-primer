#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <signal.h>
#include <sys/select.h>
#include <errno.h>

#define error_handler(msg) \
  do{ \
    printf("%s:%d %s, %s", __FILE__, __LINE__, #msg, strerror(errno)); \
  }while(0)

int pipefd[2];
static void sig_usr1(int);

int main(int argc, char **argv) {
  int nfds;
  char c;
  fd_set rdset;
  mqd_t mqd;
  void *buf;
  ssize_t n;
  unsigned int prio;
  struct mq_attr attr;
  struct sigevent sigev;

  if(argc != 2) {
	printf("usage: %s <mqname>\n", argv[0]);
	exit(1);
  }
  
  if((mqd = mq_open(argv[1], O_RDONLY | O_NONBLOCK)) == -1)
	error_handler(mq_open error);
  mq_getattr(mqd, &attr);

  if(pipe(pipefd) == -1)
	error_handler(pipe error);
  FD_ZERO(&rdset);

  if((buf = malloc(attr.mq_msgsize)) == NULL)
	error_handler(malloc error);
  signal(SIGUSR1, sig_usr1);
  sigev.sigev_notify = SIGEV_SIGNAL;
  sigev.sigev_signo = SIGUSR1;
  if(mq_notify(mqd, &sigev) == -1) {
	free(buf);
	error_handler(mq_notify error);
  }
  
  int i;
  for(i = 0; i < 3; ++i) {
	FD_SET(pipefd[0], &rdset);
	nfds = select(pipefd[0]+1, &rdset, NULL, NULL, NULL);
	if(FD_ISSET(pipefd[0], &rdset)) {
	  read(pipefd[0], &c, 1);
	  mq_notify(mqd, &sigev);
	  while(n = mq_receive(mqd, buf, attr.mq_msgsize, &prio) >= 0)
		printf("read %ld bytes, priority %d\n", (long)n, prio);
	  if(errno != EAGAIN) {
		free(buf);
		error_handler(mq_receive error);
	  }
	}
  }
  free(buf);
  return 0;
}

static void sig_usr1(int signo) {
  write(pipefd[1], "", 1); /* one byte of 0 */
  return;
}
