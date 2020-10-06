/*
 * 信号处理相关示例
 * 注册信号后，发生注册信号时，操作系统调用该信号对应的函数
 * struct sigaction {
 *   void (*sa_handler)(int);
 *   sigset_t sa_mask;
 *   int sa_flags;
 * };
 */

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

void timeout(int sig) {
  if(sig == SIGALRM)
	puts("time out");
  alarm(2);
}

int main(int argc, char **argv) {
  int i;
  struct sigaction act;
  act.sa_handler = timeout;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  sigaction(SIGALRM, &act, 0);

  alarm(2);
  for(i = 0; i < 3; ++i) {
	puts("wait...");
	sleep(30);
  }
  return 0;
}
