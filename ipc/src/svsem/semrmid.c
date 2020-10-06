#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <errno.h>

#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)

int main(int argc, char **argv) {
  int semid;
  key_t key;
  if(argc != 2) {
	printf("usage: %s <pathname>\n", argv[0]);
	exit(1);
  }
  if((key = ftok(argv[1], 0)) == -1)
	error_handler(ftok error);
  if((semid = semget(key, 0, 0)) == -1)
	error_handler(semget seeor);
  if(semctl(semid, 0, IPC_RMID) == -1)
    error_handler(semctl error);
  return 0;
}
