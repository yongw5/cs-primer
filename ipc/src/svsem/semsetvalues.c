#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/sem.h>
#include <errno.h>

#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)

union semun {
  int              val;   /* used for SETVAL */
  struct semid_ds *buf;   /* used for IPC_SET and IPC_STATE */
  unsigned short  *array; /* used for GETALL and SETALL */
};

int main(int argc, char **argv) {
  int semid, nsems, i;
  key_t key;
  struct semid_ds seminfo;
  unsigned short *ptr;
  union semun arg;
  if(argc < 2) {
	printf("usage: %s <pathname> [values ...]\n", argv[0]);
	exit(1);
  }
  /* first get the number of semaphores in the set */
  if((key = ftok(argv[1], 0)) == -1)
	error_handler(ftok error);
  if((semid = semget(key, 0, 0)) == -1)
	error_handler(semget error);
  arg.buf = &seminfo;
  if(semctl(semid, 0, IPC_STAT, arg) == -1)
	error_handler(semctl error);
  nsems = arg.buf->sem_nsems;

  /* now get the values from the commind line */
  if(argc != nsems + 2) {
	printf("%d semaphores in set, %d values specified\n", nsems, argc-2);
	exit(1);
  }

  if((ptr = calloc(nsems, sizeof(unsigned short))) == NULL)
	error_handler(calloc error);
  arg.array = ptr;
  for(i = 0; i < nsems; ++i)
	ptr[i] = atoi(argv[i+2]);
  if(semctl(semid, 0, SETALL, arg) == -1)
	error_handler(semctl error);
  free(arg.array);
  return 0;
}
