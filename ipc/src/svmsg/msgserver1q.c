#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
void server(int, int);

int main(int argc, char **argv) {
  int mqid;
  if((mqid = msgget(MQ_KEY, SVMSG_MODE | IPC_CREAT)) == -1)
	error_handler(msgget error);
  server(mqid, mqid); /*same queue for both direction */
  return 0;
}

void server(int readfd, int writefd) {
  FILE *fp;
  char *ptr;
  long type;
  ssize_t n;
  int i;
  struct msgbuf *buf;
  if((buf = malloc(MAXMSG)) == NULL)
	error_handler(malloc error);
  for(i = 0; i < 5; ++i) {
	/* read pathname from IPC channel */
	type = 1;
	if((n = msgrcv(readfd, buf, MAXMSG, type, 0)) == 0) {
	  printf("%s:%d msgrcv pathname missing\n", __FILE__, __LINE__);
	  continue;
	}
	(buf->mtext)[n] = 0;  /* null terminate pathname */
	if((ptr = strchr(buf->mtext, ' ')) == NULL) {
	  printf("bogus request: %s", buf->mtext);
	  continue;
	}

	*ptr++ = 0;  /* null terminate PID, ptr = pathname */
	buf->mtype = atol(buf->mtext);  /* for message back to client */

	if((fp = fopen(ptr, "r")) == NULL) { /* error: must tell client */
      snprintf(buf->mtext+n, MAXMSG-n, ": can't open, %s\n", strerror(errno));
	  n = strlen(ptr);
	  memmove(buf->mtext, ptr, n);
	  if(msgsnd(writefd, buf, n, 0) == -1) {
		free(buf);
		error_handler(msgsnd error);
	  }
	} else {
	  while(fgets(buf->mtext, MAXMSG, fp) != NULL) {
		n = strlen(buf->mtext);
		if(msgsnd(writefd, buf, n, 0) == -1) {
		  free(buf);
		  error_handler(msgsnd error);
		}
	  }
	  fclose(fp);
	}
	msgsnd(writefd, "", 0, 0); /* send a 0-length message to signify the end */
  }
  free(buf);
}
