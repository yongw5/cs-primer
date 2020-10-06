#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <errno.h>

#define SVSHM_MODE (SHM_R | SHM_W | SHM_R>>3 | SHM_W>>6)
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)

int main(int argc, char **argv) {
  int id;
  key_t key;

  if(argc != 2) {
	printf("usage: %s <pathname>\n", argv[0]);
	exit(1);
  }

  if((key = ftok(argv[1], 0)) == -1)
	error_handler(ftok error);
  if((id = shmget(key, 0, SVSHM_MODE)) == -1)
	error_handler(shmget error);

  if(shmctl(id, IPC_RMID, NULL) == -1)
	error_handler(shmctl error);

  return 0;
}
