/**
 * 存在数据边界的UDP套接字
 * 传输中调用IO函数的次数非常重要，输入函数和输出函数的调用次数完全一致
 * host2用于发送数据
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
  struct sockaddr_in your_addr;
  socklen_t your_addrlen;
  char msg1[] = "Hi!";
  char msg2[] = "I'm another UDP host!";
  char msg3[] = "Nice to meet you";

  if(argc != 3) {
    printf("usage: %s <ip> <port>\n", argv[0]);
	exit(1);
  }

  if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    error_handler("socket() error");
  memset(&your_addr, 0, sizeof(your_addr));
  your_addr.sin_family = AF_INET;
  your_addr.sin_addr.s_addr = inet_addr(argv[1]);
  your_addr.sin_port = htons(atoi(argv[2]));

  sendto(sockfd, msg1, sizeof(msg1), 0, (struct sockaddr*)&your_addr, sizeof(your_addr));
  sendto(sockfd, msg2, sizeof(msg2), 0, (struct sockaddr*)&your_addr, sizeof(your_addr));
  sendto(sockfd, msg3, sizeof(msg3), 0, (struct sockaddr*)&your_addr, sizeof(your_addr));
  close(sockfd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
