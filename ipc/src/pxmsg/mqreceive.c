#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char **argv) {
  int opt, flags;
  mqd_t mqd;
  ssize_t n;
  unsigned int prio;
  void *buf;
  struct mq_attr attr;

  flags = O_RDONLY;
  while((opt = getopt(argc, argv, "n")) != -1) {
	switch(opt) {
	  case 'n':
		flags |= O_NONBLOCK;
		break;
	}
  }
  if(optind != argc -1) {
	printf("usage: %s [-n] <mqname>\n", argv[0]);
	exit(1);
  }
  if((mqd = mq_open(argv[optind], flags)) == -1) {
	printf("%s :can't open, %s", argv[0], strerror(errno));
	exit(1);
  }
  mq_getattr(mqd, &attr);
  if((buf = malloc(attr.mq_msgsize)) == NULL) {
	printf("malloc error\n");
	exit(1);
  }
  if((n = mq_receive(mqd, buf, attr.mq_msgsize, &prio)) == -1) {
	printf("mq_receive() error, %s\n", strerror(errno));
	exit(1);
  }
  printf("read %ld bytes, priority=%u\n", (long)n, prio);
  free(buf);
  return 0;
}
