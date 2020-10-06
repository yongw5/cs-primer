/**
 * 验证TCP传输的数据不存在边界
 * write函数的调用次数不同于read函数的调用次数
 * client中多次调用read函数
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>

#define BUF_SZ 128
void error_handler(char *msg);

int main(int argc, char **argv) {
  int sockfd;
  struct sockaddr_in serv_addr;
  char msg[BUF_SZ];
  int msg_len;
  int idx = 0, read_len = 0;
  if(argc != 3) {
    printf("uasge: %s <IP> <PORT>\n", argv[0]);
	exit(0);
  }
  if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    error_handler("socket error");
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(atoi(argv[2]));
  if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    error_handler("connect error");
  while(read_len = read(sockfd, &msg[idx++], 1)) {
    if(read_len == -1)
	  error_handler("read error");
	msg_len += read_len;
  }
  printf("message from server: %s\n", msg);
  printf("function read call count: %d\n", msg_len);
  close(sockfd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
