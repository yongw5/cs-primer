#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <errno.h>

int main(int argc, char **argv) {
  mqd_t mqd;
  struct mq_attr attr;
  if(argc != 2) {
	printf("usage: %s <mqname>\n", argv[0]);
	exit(1);
  }

  if((mqd = mq_open(argv[1], O_RDONLY)) == -1) {
	printf("%s :can't open, %s\n", argv[1], strerror(errno));
	exit(1);
  }
  if(mq_getattr(mqd, &attr) == -1) {
	printf("mq_getarrt() error: %s", strerror(errno));
	exit(1);
  }
  printf("max #msg = %ld, max #bytes/msg = %ld, #currently on queue = %ld\n",
	  attr.mq_maxmsg, attr.mq_msgsize, attr.mq_curmsgs);
  if(mq_close(mqd) == -1) {
	printf("mq_close() error, %s\n", strerror(errno));
	exit(1);
  }
  return 0;
}
