/*
 * IO流分离以及半关闭
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
void error_handler(char *msg);

int main(int argc, char **argv) {
  int sockfd;
  struct sockaddr_in serv_addr;
  char buf[BUF_SIZE];
  FILE *readfp, *writefp;

  if(argc != 3) {
	printf("usage: %s <ip> <port>\n", argv[0]);
	exit(1);
  }

  if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
	error_handler("scoket() error");
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(atoi(argv[2]));
  if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
	error_handler("connect() error");

  readfp = fdopen(sockfd, "r");
  writefp = fdopen(sockfd, "w"); // dup to copy file descriptor
  
  while(1) {
	if(fgets(buf, sizeof(buf), readfp) == NULL)
	  break;
	fputs(buf, stdout);
	fflush(stdout);
  }

  fputs("FROM CLIENT: Thank you?\n", writefp);
  fflush(writefp);
  fclose(writefp);
  fclose(readfp);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
