/*
 * epoll 边缘触发（Level Trigger）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define BUF_SIZE 4
#define EPOLL_SIZE 50
void setnonblockingmode(int fd);
void error_handler(char *msg);

int main(int argc, char **argv) {
  int serv_sockfd, clnt_sockfd;
  struct sockaddr_in serv_addr, clnt_addr;
  socklen_t clnt_addrlen;
  char buf[BUF_SIZE];
  int str_len, i;

  struct epoll_event ep_events[EPOLL_SIZE];
  struct epoll_event event;
  int epfd, event_cnt;
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
	error_handler("bind() error");

  epfd = epoll_create(EPOLL_SIZE);
  setnonblockingmode(serv_sockfd);
  event.events = EPOLLIN;
  event.data.fd = serv_sockfd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sockfd, &event);

  while(1) {
	event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
	if(event_cnt == -1) {
	  puts("epoll_wait() error");
	  break;
	}
	puts("return epoll_wait");
	for(i = 0; i < event_cnt; ++i) {
	  if(ep_events[i].data.fd == serv_sockfd) {
		clnt_addrlen = sizeof(clnt_addr);
		clnt_sockfd = accept(serv_sockfd, (struct sockaddr*)&clnt_addr, &clnt_addrlen);
		setnonblockingmode(clnt_sockfd);
		event.events = EPOLLIN | EPOLLET;
		event.data.fd = clnt_sockfd;
		epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sockfd, &event);
		printf("connected client: %d\n", clnt_sockfd);
	  } else {
		while(1) {
		  str_len = read(ep_events[i].data.fd, buf, BUF_SIZE);
		  if(str_len == 0) { // close request
            epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
			close(ep_events[i].data.fd);
			printf("closed client: %d\n", ep_events[i].data.fd);
			break;
		  } else if(str_len < 0) { // no data to read
			if(errno == EAGAIN);
			break;
		  } else {
			write(ep_events[i].data.fd, buf, str_len);
		  }
		}
	  }
	}
  }
  close(serv_sockfd);
  close(epfd);
  return 0;
}

void setnonblockingmode(int fd) {
  int flag = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flag|O_NONBLOCK);
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
