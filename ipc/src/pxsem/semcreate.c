#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <errno.h>

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)

int main(int argc, char **argv) {
  int opt, flags;
  sem_t *sem;
  unsigned int value;

  flags = O_RDWR | O_CREAT;
  value = 1;

  while((opt = getopt(argc, argv, "ei:")) != -1) {
	switch(opt) {
	  case 'e':
		flags |= O_EXCL;
		break;
	  case 'i':
		value = atoi(optarg);
		break;
	}
  }
  if(optind != argc -1) {
	printf("usage: %s [-e] [-i intivalue] <name>\n", argv[0]);
	exit(1);
  }
  if((sem = sem_open(argv[optind], flags, FILE_MODE, value)) == SEM_FAILED)
	error_handler(sem_open error);
  if(sem_close(sem) == -1)
	error_handler(sem_close error);
  return 0;
}
