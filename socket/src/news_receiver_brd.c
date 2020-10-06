/*
 * 实现广播Sender和Receiver
 * Receiver：接受×××组的新闻信息
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30
void error_handler(char *msg);

int main(int argc, char **argv) {
  int recv_sockfd;
  struct sockaddr_in addr;
  char buf[BUF_SIZE];
  int str_len;

  if(argc != 3) {
	printf("usage: %s <PORT>\n", argv[0]);
	exit(1);
  }

  if((recv_sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
	error_handler("socket() error");
  memset(&recv_addr, 0, sizeof(recv_addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(atoi(argv[1]));

  if(bind(recv_sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
	error_handler("bind() error");

  while(1) {
	if((str_len = recvfrom(recv_sockfd, buf, BUF_SIZE-1, 0, NULL, 0)) <= 0)
	  break;
	buf[str_len] = 0;
	fputs(buf, stdout);
  }
  close(recv_sockfd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
