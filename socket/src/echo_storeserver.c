/*
 * 保存消息的回声服务端
 * 将保存消息委托给另外的进程，利用pipe通信
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define BUF_SIZE 100
void error_handler(char *msg);
void read_childproc(int sig);

int main(int argc, char **argv) {
  int serv_sockfd, clnt_sockfd;
  struct sockaddr_in serv_addr, clnt_addr;
  socklen_t clnt_addrlent;
  struct sigaction act;
  pid_t pid;
  char buf[BUF_SIZE];
  int buf_len, status;
  int fds[2];

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
  pipe(fds);
  if((pid = fork()) == -1)
	error_handler("fork() error");
  if(pid == 0) {
	FILE *fp = fopen("echomsg.txt", "wt");
	char msgbuf[BUF_SIZE];
	int i, len;

	for(i = 0; i < 3; i++) {
	  len = read(fds[0], msgbuf, BUF_SIZE);
	  fwrite((void*)msgbuf, 1, len, fp);
	}
	fclose(fp);
	return 0;
  }
  while(1) {
	clnt_addrlent = sizeof(clnt_addr);
	if((clnt_sockfd = accept(serv_sockfd, (struct sockaddr*)&clnt_addr, &clnt_addrlent)) == -1)
	  continue;
	else
	  puts("new client connected...");
	pid = fork();
	if(pid == 0) {
	  close(serv_sockfd);
	  while((buf_len = read(clnt_sockfd, buf, BUF_SIZE)) != 0) {
		write(clnt_sockfd, buf, buf_len);
		write(fds[1], buf, buf_len);
	  }
	  close(clnt_sockfd);
	  puts("client disconnected...");
	  return 0;
	} else
	  close(clnt_sockfd);
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
