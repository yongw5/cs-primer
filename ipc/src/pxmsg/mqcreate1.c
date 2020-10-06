#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int main(int argc, char **argv) {
  int c, flags;
  mqd_t mqd;
  flags = O_RDWR | O_CREAT;

  if(argc != 2) {
	printf("usage: %s mqname\n", argv[0]);
	exit(1);
  }
  if((mqd = mq_open(argv[1], flags, FILE_MODE, NULL)) == -1) {
	printf("%s: can\t open, %s\n", argv[0], strerror(errno));
	exit(1);
  }
  close(mqd);
  return 0;
}
