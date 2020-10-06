/**
 * UDP回声服务端/客户端示例
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
  struct sockaddr_in serv_addr, from_addr;
  socklen_t from_addrlen;
  char msg[BUF_SIZE];
  int msg_len;
  if(argc != 3) {
    printf("usage: %s <IP> <port>\n", argv[0]);
	exit(1);
  }

  if((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
    error_handler("socket error");
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(atoi(argv[2]));

  while(1) {
    fputs("Input message(q to quit): ", stdout);
	fgets(msg, sizeof(msg), stdin);
	if(!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
      break;
	sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	from_addrlen = sizeof(from_addr);
	msg_len = recvfrom(sockfd, msg, BUFSIZ, 0, (struct sockaddr*)&from_addr, &from_addrlen);
	msg[msg_len] = 0;
	fputs(msg, stdout);
  }
  close(sockfd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
