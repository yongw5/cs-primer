/**
 * 计算器服务端/客户端示例
 * 客户端连接到服务端后以1字节整数形式传递待运算数字个数
 * 客户端向服务器端传递的每个整型数据占4字节
 * 传递整数后接着传递运算符。运算符信息占用1字节
 * 选择字符+、-、*之一传递
 * 服务端得到运算结果后向客户端传回运算结果
 * 客户端得到运算结果后终止与服务器端的连接
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define BUF_SIZE 1024
#define RLT_SIZE 4
#define OPSZ 4
void error_handler(char *msg);

int main(int argc, char **argv) {
  int sockfd;
  struct sockaddr_in serv_addr;
  char opmsg[BUF_SIZE];
  int result, opnd_cnt, i;
  if(argc != 3) {
    printf("usage: %s <IP> <port\n>", argv[0]);
	exit(1);
  }
  if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    error_handler("socket error");
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(atoi(argv[2]));
  if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    error_handler("connect error");
  fputs("Operand count: ", stdout);
  scanf("%d", &opnd_cnt);
  opmsg[0] = (char)opnd_cnt;
  for(i = 0; i < opnd_cnt; ++i) {
    printf("Operand %d: ", i+1);
	scanf("%d", (int*)&opmsg[i*OPSZ+1]);
  }
  fgetc(stdin);
  fputs("Operator: ", stdout);
  scanf("%c", &opmsg[opnd_cnt*OPSZ+1]);
  write(sockfd, opmsg, opnd_cnt*OPSZ+2);
  read(sockfd, &result, RLT_SIZE);

  printf("Operation result: %d\n", result);
  close(sockfd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
