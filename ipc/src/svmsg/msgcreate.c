#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/errno.h>

#define MSG_R 0400
#define MSG_W 0200
#define SVMSG_MODE (MSG_R | MSG_W | MSG_R>>3 | MSG_R>>6)
#define error_handler(msg) \
  do { \
    printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1);\
  }while(0)

int main(int argc, char **argv) {
  int opt, oflag, mqid;
  key_t key;
  oflag = SVMSG_MODE | IPC_CREAT;
  while((opt = getopt(argc, argv, "e")) != -1) {
	switch(opt) {
	  case 'e':
		oflag |= IPC_EXCL;
		break;
	}
  }
  if(optind != argc - 1) {
	printf("usage: %s [-e] <pathname>\n", argv[0]);
	exit(1);
  }
  if((key = ftok(argv[optind], 0)) == -1)
	error_handler(ftok error);
  if((mqid = msgget(key, oflag)) == -1)
	error_handler(msgget error);
  return 0;
}
