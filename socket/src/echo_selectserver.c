/*
 * 用select实现IO复用回声服务器
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>

#define BUF_SIZE 100
void error_handler(char *msg);

int main(int argc, char **argv) {
  int serv_sockfd, clnt_sockfd;
  struct sockaddr_in serv_addr, clnt_addr;
  socklen_t clnt_addrlen;
  fd_set reads, cpy_reads;
  struct timeval timeout;
  int fd_max, str_len, fd_num, i;
  char buf[BUF_SIZE];

  if(argc != 2) {
	printf("usage: %s <port>\n", argv[0]);
	exit(1);
  }

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

  FD_ZERO(&reads);
  FD_SET(serv_sockfd, &reads);
  fd_max = serv_sockfd;

  while(1) {
	cpy_reads = reads;
	timeout.tv_sec = 5;
	timeout.tv_usec = 5000;
    
	if((fd_num = select(fd_max+1, &cpy_reads, 0, 0, &timeout)) == -1)
	  break;
	if(fd_num == 0)
	  continue;
	for(i = 0; i < fd_max+1; ++i) {
	  if(FD_ISSET(i, &cpy_reads)) {
		if(i == serv_sockfd) { // connection request!
          clnt_addrlen = sizeof(clnt_addr);
		  if((clnt_sockfd = accept(serv_sockfd, (struct sockaddr*)&clnt_addr, &clnt_addrlen)) == -1)
			continue;
		  FD_SET(clnt_sockfd, &reads);
		  if(fd_max < clnt_sockfd)
			fd_max = clnt_sockfd;
		  printf("connected client: %d\n", clnt_sockfd);
		} else {
		  str_len = read(i, buf, BUF_SIZE);
		  if(str_len == 0) { // EOF, close request
		    FD_CLR(i, &reads);
			close(i);
			printf("closed client: %d", i);
		  } else {
			write(i, buf, str_len); // echo
		  }
		}
	  }
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
