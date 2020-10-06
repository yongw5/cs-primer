/**
 * TCP 回声客户端
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
  char msg[BUF_SIZE];
  int msg_len, recv_len;
  struct sockaddr_in serv_addr;
  if(argc != 3) {
    printf("usage: %s <IP> <PORT>\n", argv[0]);
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
  while(1) {
    puts("input message(q to quit):");
    msg_len = 0;
    do{
      recv_len = read(0, msg, BUF_SIZE);
      if(recv_len != BUF_SIZE) {
      msg[recv_len] = 0;
      if(!strcmp(msg, "q\n") || !strcmp(msg, "Q\n")) {
        close(sockfd);
        return 0;
      }
      msg_len += recv_len;
      write(sockfd, msg, recv_len);
      break;
      }
      msg_len += recv_len;
      write(sockfd, msg, recv_len);
    } while(1);
    puts("message from server:");
    recv_len = 0;
    while(recv_len < msg_len) {
      int recv_cnt = read(sockfd, msg, BUF_SIZE-1);
      if(recv_cnt == -1)
      error_handler("read error");
      msg[recv_cnt] = 0;
      recv_len += recv_cnt;
      fputs(msg, stdout);
    }
  }
  printf("\n");
  close(sockfd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
