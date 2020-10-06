/**
 * 计算器服务端/客户端示例
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
#define OPSZ 4
void error_handler(char *msg);
int calculate(int opnum, int opnds[], char operator);

int main(int argc, char **argv) {
  int serv_sockfd, clnt_sockfd;
  struct sockaddr_in serv_addr, clnt_addr;
  socklen_t clnt_addrlen;
  char opinfo[BUF_SIZE];
  int result, opnd_cnt, i, recv_cnt, recv_len;
  if(argc != 2) {
    printf("usage: %s <PORT>\n", argv[0]);
	exit(1);
  }

  if((serv_sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    error_handler("socket error");
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htons(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));

  if(bind(serv_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    error_handler("bind error");
  if(listen(serv_sockfd, 5) == -1)
    error_handler("listen error");
  clnt_addrlen = sizeof(clnt_addr);
  for(i = 0; i < 5; ++i) {
    opnd_cnt = 0;
	clnt_sockfd = accept(serv_sockfd, (struct sockaddr*)&clnt_addr, &clnt_addrlen);
	read(clnt_sockfd, &opnd_cnt, 1);

	recv_len = 0;
	while(recv_len < (opnd_cnt*OPSZ+1)) {
	  recv_cnt = read(clnt_sockfd, &opinfo[recv_len], BUF_SIZE-1);
	  recv_len += recv_cnt;
	}
	result = calculate(opnd_cnt, (int*)opinfo, opinfo[recv_len-1]);
	write(clnt_sockfd, (char*)&result, sizeof(result));
	close(clnt_sockfd);
  }
  close(serv_sockfd);
  return 0;
}

int calculate(int opnum, int opnds[], char op) {
  int result = opnds[0], i;
  switch(op) {
    case '+':
	  for(i = 1; i < opnum; ++i)
	    result += opnds[i];
	  break;
	case '-':
	  for(i = 1; i < opnum; ++i)
	    result -= opnds[i];
	  break;
	case '*':
	  for(i = 1; i < opnum; ++i)
	    result *= opnds[i];
	  break;
  }
  return result;
}
void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
