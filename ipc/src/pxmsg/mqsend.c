#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char **argv) {
  mqd_t mqd;
  void *ptr;
  size_t len;
  unsigned int prio;

  if(argc != 4) {
	printf("usage: %s <mq_name> <#nbytes> <priority>\n", argv[0]);
	exit(1);
  }
  len = atoi(argv[2]);
  prio = atoi(argv[3]);

  if((mqd = mq_open(argv[1], O_WRONLY)) == -1) {
	printf("%s :can't open, %s", argv[1], strerror(errno));
	exit(1);
  }

  ptr = calloc(len, sizeof(char));
  if(mq_send(mqd, ptr, len, prio) == -1) {
	printf("mq_send() error, %s\n", strerror(errno));
	exit(1);
  }
  free(ptr);
  return 0;
}
