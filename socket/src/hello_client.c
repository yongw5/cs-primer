/**
 * 调用socket函数和connect函数
 * 与服务端共同运行接受字符串数据
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void error_handler(char *msg);

int main(int argc, char **argv) {
  int sockfd;
  struct sockaddr_in serv_addr;
  char msg[30];
  int str_len;

  if(argc != 3) {
    printf("usage: %s <IP> <port>\n", argv[0]);
	  exit(0);
  }
  
  sockfd = socket(PF_INET, SOCK_STREAM, 0);
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(atoi(argv[2]));

  if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    error_handler("connect() error");
  str_len = read(sockfd, msg, sizeof(msg)-1);
  if(str_len == -1)
    error_handler("read() error");
  printf("message from server: %s\n", msg);
  close(sockfd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
