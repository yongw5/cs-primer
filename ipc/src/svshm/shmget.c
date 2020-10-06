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
  int opt, id, oflag;
  key_t key;
  char *ptr;
  size_t length;

  oflag = SVSHM_MODE | IPC_CREAT;
  while((opt = getopt(argc, argv, "e")) != -1)
	switch(opt) {
	  case 'e':
		oflag |= IPC_EXCL;
		break;
	}
  if(optind != argc - 2) {
	printf("usage: %s [-e] <pathname> <length>\n", argv[0]);
	exit(1);
  }
  length = atoi(argv[optind+1]);

  if((key = ftok(argv[optind], 0)) == -1)
	error_handler(ftok error);
  if((id = shmget(key, length, oflag)) == -1)
	error_handler(shmget error);

  if((ptr = shmat(id, NULL, 0)) == (void*)-1)
	error_handler(shmat error);
  return 0;
}
