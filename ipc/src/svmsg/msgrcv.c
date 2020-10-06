#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/errno.h>

#define MSG_W 0200
#define MAXMSG (8192 + sizeof(long))
#define error_handler(msg) \
  do{ \
    printf("%s:%d %s %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)

int main(int argc, char **argv) {
  int opt, flag, mqid;
  key_t key;
  long type;
  ssize_t n;
  struct msgbuf *buf;

  type = flag = 0;
  while((opt = getopt(argc, argv, "nt:")) != -1) {
	switch(opt) {
	  case 'n':
		flag |= IPC_NOWAIT;
		break;
	  case 't':
		type = atol(optarg);
		break;
	}
  }
  if(optind != argc -1) {
	printf("usage: %s [-n] [-t type] <pathname>\n", argv[0]);
	exit(1);
  }
  if((key = ftok(argv[optind], 0)) == -1)
	error_handler(ftok error);
  if((mqid = msgget(key, 0)) == -1)
	error_handler(msgget error);
  if((buf = malloc(MAXMSG)) == NULL)
	error_handler(malloc error);
  if((n = msgrcv(mqid, buf, MAXMSG, type, flag)) == -1)
	error_handler(msgrcv error);
  printf("read %d bytes, type = %ld\n", n, buf->mtype);
  return 0;
}
