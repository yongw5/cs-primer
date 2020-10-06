/*
 * MSG_OOB：发生紧急消息
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#define BUF_SIZE 30
void error_handler(char *msg);
void urg_handler(int signo);

int serv_sockfd, clnt_sockfd;

int main(int argc, char **argv) {
  struct sockaddr_in serv_addr, clnt_addr;
  socklen_t clnt_addrlen;
  struct sigaction act;
  int str_len, state;
  char buf[BUF_SIZE];

  if(argc != 2) {
	printf("usage: %s <port>\n", argv[0]);
	exit(1);
  }

  act.sa_handler = urg_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if((serv_sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
	error_handler("socket() error");
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));

  if(bind(serv_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
	error_handler("bind() error");

  if(listen(serv_sockfd, 5) == -1)
	error_handler("listen() error");

  clnt_addrlen = sizeof(clnt_sockfd);
  if((clnt_sockfd = accept(serv_sockfd, (struct sockaddr*)&clnt_addr, &clnt_addrlen)) == -1)
	error_handler("accept() error");

  fcntl(clnt_sockfd, F_SETOWN, getpid());
  state = sigaction(SIGURG, &act, 0);
  while((str_len = recv(clnt_sockfd, buf, BUF_SIZE, 0)) != 0) {
	if(str_len == -1)
	  continue;
	buf[str_len] = 0;
	puts(buf);
  }
  close(clnt_sockfd);
  close(serv_sockfd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}

void urg_handler(int sig) {
  int str_len;
  char buf[BUF_SIZE];
  str_len = recv(clnt_sockfd, buf, BUF_SIZE-1, MSG_OOB);
  buf[str_len] = 0;
  printf("urgent message: %s\n", buf);
}
