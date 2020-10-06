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

int main(int argc, char **argv) {
  int opt, i, flag, semid, nops;
  key_t key;
  struct sembuf *ptr;

  flag = 0;
  while((opt = getopt(argc, argv, "nu")) != -1) {
	switch(opt) {
	  case 'n':
		flag |= IPC_NOWAIT;
		break;
	  case 'u':
		flag |= SEM_UNDO;
		break;
	  default:
		break;
	}
  }

  if(argc - optind < 2) {
	printf("usage: %s [-n] [-u] <pathname> operation ...\n", argv[0]);
	exit(1);
  }

  if((key = ftok(argv[optind], 0)) == -1)
	error_handler(ftok error);
  if((semid = semget(key, 0, 0)) == -1)
	error_handler(semget error);
  --optind;
  nops = argc - optind;
  
  /* allocate memory to hold operations, store, and perform */
  if((ptr = calloc(nops, sizeof(struct sembuf))) == NULL)
	error_handler(calloc error);
  for(i = 0; i < nops; ++i) {
	ptr[i].sem_num = i;
	ptr[i].sem_op = atoi(argv[optind + 0]);
	ptr[i].sem_flg = flag;
  }
  if(semop(semid, ptr, nops) == -1)
	error_handler(semop error);
  free(ptr);
  return 0;
}
