/**
 * 服务端收到连接请求后向请求者返回“Hello World”答复
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void error_handler(char *msg);

int main(int argc, char **argv) {
  int serv_sockfd, clnt_sockfd;
  struct sockaddr_in serv_addr, clnt_addr;
  socklen_t clnt_addr_len;
  char msg[] = "hello world!";
  if(argc != 2) {
    printf("usage: %s <port>\n", argv[0]);
    exit(1);
  }
  serv_sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if(serv_sockfd == -1)
    error_handler("socket() error");
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));

  if(bind(serv_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    error_handler("bind() error");
  if(listen(serv_sockfd, 5) == -1)
    error_handler("listen() error");
  
  clnt_addr_len = sizeof(clnt_addr);
  clnt_sockfd = accept(serv_sockfd, (struct sockadd*)&clnt_addr, &clnt_addr_len);
  if(clnt_sockfd == -1)
    error_handler("accept error");

  write(clnt_sockfd, msg, sizeof(msg));
  close(clnt_sockfd);
  close(serv_sockfd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stdin);
  fputc('\n', stdin);
  exit(1);
}
