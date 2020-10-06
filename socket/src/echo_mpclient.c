/*
 * 多进程实现并发服务器
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30
void error_handler(char *msg);
void read_routine(int sock, char *buf);
void write_routine(int sock, char *buf);

int main(int argc, char **argv) {
  int sockfd;
  struct sockaddr_in serv_addr;
  pid_t pid;
  char buf[BUF_SIZE];

  if(argc != 3) {
	printf("usage: %s <ip> <port>\n", argv[0]);
	exit(1);
  }

  if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
	error_handler("socket() error");
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(atoi(argv[2]));

  if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
	error_handler("connect() error");

  if((pid = fork()) == -1)
	error_handler("fork() error");
  if(pid == 0)
	write_routine(sockfd, buf);
  else
	read_routine(sockfd, buf);
  close(sockfd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}

void read_routine(int sock, char *buf) {
  int msg_len;
  while(1) {
	if((msg_len = read(sock, buf, BUF_SIZE)) == 0)
	  return;
	buf[msg_len] = 0;
	printf("message from server: %s\n", buf);
  }
}

void write_routine(int sock, char *buf) {
  while(1) {
	fgets(buf, BUF_SIZE, stdin);
	if(!strcmp("q\n", buf) || !strcmp("Q\n", buf)) {
	  shutdown(sock, SHUT_WR);
	}
	write(sock, buf, strlen(buf));
  }
}
