/*
 * MSG_OOB：发生紧急消息
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUF_ZISE 30
void error_handler(char *msg);

int main(int argc, char **argv) {
  int sockfd;
  struct sockaddr_in recv_addr;
  if(argc != 3) {
	printf("usage: %s <ip> <port>\n", argv[0]);
	exit(1);
  }

  if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
	error_handler("socket() error");
  memset(&recv_addr, 0, sizeof(recv_addr));
  recv_addr.sin_family = AF_INET;
  recv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  recv_addr.sin_port = htons(atoi(argv[2]));

  if(connect(sockfd, (struct sockaddr*)&recv_addr, sizeof(recv_addr)) == -1)
	error_handler("connect() error");

  write(sockfd, "123", strlen("123"));
  send(sockfd, "4", strlen("4"), MSG_OOB);
  write(sockfd, "567", strlen("567"));
  send(sockfd, "890", strlen("890"), MSG_OOB);
  close(sockfd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
