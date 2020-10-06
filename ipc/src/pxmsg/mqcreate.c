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
  struct mq_attr attr;
  char opt;
  memset(&attr, 0, sizeof(attr));

  flags = O_RDWR | O_CREAT;
  while((opt = getopt(argc, argv, "em:z:")) != -1) {
	switch(opt) {
	  case 'e':
		flags |= O_EXCL;
		break;
	  case 'm':
		attr.mq_maxmsg = atol(optarg);
		break;
	  case 'z':
		attr.mq_msgsize = atol(optarg);
		break;
	}
  }
  if(optind != argc -1) {
	printf("usage: %s [-e] [-m maxmsg -z msgsize] <name>\n", argv[0]);
	exit(1);
  }
  if((attr.mq_maxmsg != 0 && attr.mq_msgsize == 0) || 
	  (attr.mq_maxmsg == 0 && attr.mq_msgsize != 0)) {
	printf("must specify both -m maxmsg end -z msgsize\n");
	exit(1);
  }

  if((mqd = mq_open(argv[optind], flags, FILE_MODE, 
		  (attr.mq_maxmsg != 0) ? &attr : NULL)) == -1) {
	printf("%s: can't open, %s\n", argv[0], strerror(errno));
	exit(1);
  }
  close(mqd);
  return 0;
}
