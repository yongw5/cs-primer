/*
 * 多进程实现并发服务器
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>

#define BUF_SIZE 30
void error_handler(char *msg);
void read_childproc(int sig);

int main(int argc, char **argv) {
  int serv_sockfd, clnt_sockfd;
  struct sockaddr_in serv_addr, clnt_addr;
  socklen_t clnt_addrlen;

  pid_t pid;
  struct sigaction act;
  int state, msg_len;
  char msg[BUF_SIZE];

  if(argc != 2) {
	printf("usage: %s <port>\n", argv[0]);
	exit(1);
  }
  
  act.sa_handler = read_childproc;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  sigaction(SIGCHLD, &act, 0);

  if((serv_sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
	error_handler("socket() error");
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));
  
  if(bind(serv_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
	error_handler("bind() error");

  if(listen(serv_sockfd, 5) == -1)
	error_handler("listen() error");

  while(1) {
	clnt_addrlen = sizeof(clnt_addr);
	if((clnt_sockfd = accept(serv_sockfd, (struct sockaddr*)&clnt_addr, &clnt_addrlen)) == -1)
	  continue;
	else
	  puts("new client connected...");
	if((pid = fork()) == -1) {
	  close(clnt_sockfd);
	  continue;
	}
	if(pid == 0) {
	  close(serv_sockfd);
	  while((msg_len = read(clnt_sockfd, msg, BUF_SIZE)) != 0)
		write(clnt_sockfd, msg, msg_len);
	  close(clnt_sockfd);
	  puts("client disconnected...");
	  return 0;
	} else {
	  close(clnt_sockfd);
	}
  }
  close(serv_sockfd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}

void read_childproc(int sig) {
  int status;
  pid_t pid = waitpid(-1, &status, WNOHANG);
  printf("removed proc id: %d\n", pid);
}
