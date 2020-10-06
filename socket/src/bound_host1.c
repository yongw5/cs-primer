/**
 * 存在数据边界的UDP套接字
 * 传输中调用IO函数的次数非常重要，输入函数和输出函数的调用次数完全一致
 * host1用于接收数据
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
  int sockfd;
  struct sockaddr_in my_addr, your_addr;
  socklen_t addr_sz;
  char msg[BUF_SIZE];
  int msg_len, i;

  if(argc != 2) {
    printf("usage: %s <port>\n", argv[0]);
	exit(1);
  }

  if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    error_handler("socket() error");
  memset(&my_addr, 0, sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  my_addr.sin_port = htons(atoi(argv[1]));
  if(bind(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1)
    error_handler("bind() error");
  for(int i =0; i < 3; ++i) {
    sleep(5);
	addr_sz = sizeof(your_addr);
	msg_len = recvfrom(sockfd, msg, BUF_SIZE, 0, (struct sockaddr*)&your_addr, &addr_sz);
	printf("message %d: %s\n", i+1, msg);
  }
  close(sockfd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
