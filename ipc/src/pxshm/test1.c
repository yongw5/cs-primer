/**
 * shared memory can appear at different address in dirrerent processes 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <wait.h>
#include <errno.h>

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)

int main(int argc, char **argv) {
  int fd1, fd2, *ptr1, *ptr2;
  pid_t childpid;
  struct stat stat;

  if(argc != 3) {
	printf("usage: %s <name1> <name2>\n", argv[0]);
	exit(1);
  }

  if((shm_unlink(argv[1]) == -1) && errno != ENOENT)
	error_handler(shm_unlink error);
  if((fd1 = shm_open(argv[1], O_RDWR | O_CREAT | O_EXCL, FILE_MODE)) == -1)
	error_handler(shm_open error);
  if(ftruncate(fd1, sizeof(int)) == -1)
	error_handler(ftruncate error);
  if((fd2 = open(argv[2], O_RDONLY)) == -1)
	error_handler(open error);
  if(fstat(fd2, &stat) == -1)
	error_handler(fstat error);

  if((childpid = fork()) == 0) {
	/*child*/
	if((ptr2 = mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd2, 0)) == MAP_FAILED)
	  error_handler(mmap error);
	if((ptr1 = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd1, 0)) == MAP_FAILED)
	  error_handler(mmap error);
	printf("child: shm ptr = %p, motd ptr = %p\n", ptr1, ptr2);

	sleep(5);
	printf("shared memory interget = %d\n", *ptr1);
	return 0;
  }

  /* parent: mmap in reverse order from child */ 
  if((ptr1 = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd1, 0)) == MAP_FAILED)
	error_handler(mmap error);
  if((ptr2 = mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd2, 0)) == MAP_FAILED)
	error_handler(mmap error);
  printf("parent shm ptr = %p, motd ptr = %p\n", ptr1, ptr2);
  *ptr1 = 777;
  waitpid(childpid, NULL, 0);
  return 0;
}
