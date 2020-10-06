/**
 * memory mapping when mmap equals file size
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)
#define max(a, b) ((a) > (b) ? (a) : (b))

int main(int argc, char **argv) {
  int fd, i;
  char *ptr;
  size_t filesize, mmapsize, pagesize;

  if(argc != 4) {
	printf("usage: %s <pathname> <filesize> <mmapsize>\n", argv[0]);
	exit(1);
  }
  filesize = atoi(argv[2]);
  mmapsize = atoi(argv[3]);

  /* open file: create or truncate; set file size */
  if((fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, FILE_MODE)) == -1)
	error_handler(open error);
  lseek(fd, filesize-1, SEEK_SET);
  write(fd, "", 1);

  if((ptr = mmap(NULL, mmapsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
	error_handler(mmap error);
  close(fd);

  pagesize = sysconf(_SC_PAGESIZE);
  printf("_SC_PAGESIZE = %ld\n", (long)pagesize);

  for(i = 0; i < max(filesize, mmapsize); i+=pagesize) {
	printf("ptr[%d] = %d\n", i, ptr[i]);
	ptr[i] = 1;
	printf("ptr[%d] = %d\n", i+pagesize-1, ptr[i+pagesize-1]);
	ptr[i+pagesize-1] = 1;
  }
  printf("ptr[%d] = %d\n", i, ptr[i]);
  return 0;
}

