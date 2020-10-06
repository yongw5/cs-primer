#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)

int main(int argc, char **argv) {
  int i, fd;
  unsigned char *ptr;
  struct stat stat;

  if(argc != 2) {
	printf("usage: %s <name>\n", argv[0]);
	exit(1);
  }
  if((fd = shm_open(argv[1], O_RDWR, FILE_MODE)) == -1)
	error_handler(shm_open error);
  if(fstat(fd, &stat) == -1)
	error_handler(fstat error);
  if((ptr = mmap(NULL, stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
	error_handler(mmap error);
  close(fd);

  for(i = 0; i < stat.st_size; ++i)
	*ptr++ = i % 256;
  return 0;
}
