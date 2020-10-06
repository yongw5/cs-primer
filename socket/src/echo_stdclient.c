/*
 * 基于套接字的标准IO函数使用
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
  char msg[BUF_SIZE];
  int msg_len;
  FILE *readfp, *writefp;

  if(argc != 3) {
	printf("usage: %s <ip> <port>\n", argv[0]);
	exit(1);
  }

  if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
	error_handler("socket() error");
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(atoi(argv[2]));

  if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
	error_handler("connect() error");
  fputs("connected.......\n", stdout);

  readfp = fdopen(sockfd, "r");
  writefp = fdopen(sockfd, "w");
  while(1) {
	fputs("input message(q to quit): ", stdout);
	fgets(msg, BUF_SIZE, stdin);
	if(!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
	  break;
	fputs(msg, writefp);
	fflush(writefp);
	fgets(msg, BUF_SIZE, readfp);
	printf("message from server: %s", msg);
  }
  fclose(readfp);
  fclose(writefp);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
