#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)

int main(int argc, char **argv) {
  if(argc != 2) {
	printf("usage: %s <name>\n", argv[0]);
	exit(1);
  }
  if(shm_unlink(argv[1]) == -1)
	error_handler(shm_unlink error);

  return 0;
}
