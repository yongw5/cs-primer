#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <errno.h>

int main(int argc, char **argv) {
  if(argc != 2) {
	printf("usage: %s <mqname>", argv[0]);
	exit(1);
  }
  if(mq_unlink(argv[1]) == -1) {
	printf("%s: can't unlink, %s\n", argv[1], strerror(errno));
	exit(1);
  }
  return 0;
}
