/**
 * UDP套接字调用connect函数向套接字注册目标IP的端口信息
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
  struct sockaddr_in serv_addr;
  socklen_t serv_addrlen;
  char msg[BUF_SIZE];
  int msg_len;

  if(argc != 3) {
    printf("usage: %s <ip> <port>\n", argv[0]);
	exit(1);
  }
  if((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
    error_handler("socket() error");
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(atoi(argv[2]));

  connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

  while(1) {
    fputs("input message (q to quite): ", stdout);
	fgets(msg, sizeof(msg), stdin);
	if(!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
	  break;
	write(sockfd, msg, strlen(msg));
    
	msg_len = read(sockfd, msg, BUF_SIZE-1);
	msg[msg_len] = 0;
	printf("message from server: %s", msg);
  }
  close(sockfd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
