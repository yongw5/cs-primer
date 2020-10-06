#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/errno.h>

#define MSG_R 0400
#define MSG_W 0200 
#define SVMSG_MODE (MSG_R | MSG_W | MSG_R>>3 | MSG_W>>6)
#define MQ_KEY 1234L
#define MAXMSG (8192 + sizeof(long))
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)
void sig_chld(int);

int main(int argc, char **argv) {
  int readfd, writefd, i;
  struct msgbuf *buf;
  FILE *fp;
  size_t len;
  ssize_t n;
  char *ptr;

  signal(SIGCHLD, sig_chld);

  if((buf = malloc(MAXMSG)) == NULL)
	error_handler(malloc error);
  if((readfd = msgget(MQ_KEY, SVMSG_MODE | IPC_CREAT)) == -1)
	error_handler(msgget error);
  for(i = 0; i < 5; ++i) {
	if((n = msgrcv(readfd, buf, MAXMSG, 1, 0)) == 0) {
	  printf("pathname missing\n");
	  continue;
	}
	(buf->mtext)[n] = 0;
	if((ptr = strchr(buf->mtext, ' ')) == NULL) {
	  printf("bogus request: %s\n", buf->mtext);
	  continue;
	}
	*ptr++ = 0;
	writefd = atol(buf->mtext);
	if(fork() == 0) {
	  if((fp = fopen(ptr, "r")) == NULL) {
		snprintf(buf->mtext+n, MAXMSG-n, ": can't open, %s\n", strerror(errno));
		len = strlen(ptr);
		memmove(buf->mtext, ptr, len);
		msgsnd(writefd, buf, len, 0);
	  } else {
		while(fgets(buf->mtext, MAXMSG, fp) != NULL) {
		  len = strlen(buf->mtext);
		  msgsnd(writefd, buf, len, 0);
		}
		fclose(fp);
	  }
	  return 0;
    }
  }
  free(buf);
  return 0;
}

void sig_chld(int signo) {
  pid_t pid;
  while((pid = waitpid(-1, NULL, WNOHANG)) > 0);
  return;
}
