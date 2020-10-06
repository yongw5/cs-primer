#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)

int main(int argc, char **argv) {
  int opt, fd, flags;
  char *ptr;
  off_t length;

  flags = O_RDWR | O_CREAT;
  while((opt = getopt(argc, argv, "e")) != -1) {
	switch(opt) {
	  case 'e':
		flags |= O_EXCL;
		break;
	}
  }
  if(optind != argc - 2) {
	printf("usage: %s [-e] <name> <length>\n", argv[0]);
	exit(1);
  }
  length = atoi(argv[optind+1]);
  if((fd = shm_open(argv[optind], flags, FILE_MODE)) == -1)
	error_handler(shm_open error);
  if(ftruncate(fd, length) == -1)
	error_handler(ftruncate error);
  if((ptr = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
	error_handler(mmap error);

  return 0;
}
