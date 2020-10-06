#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

typedef void (*sa_sigaction_ptr)(int, siginfo_t *, void *);
sa_sigaction_ptr signal_rt(int signo, sa_sigaction_ptr func, sigset_t *mask);

static void sig_rt(int, siginfo_t *, void *);

int main(int argc, char **argv) {
  int i, j;
  pid_t pid;
  sigset_t newset;
  union sigval val;
  printf("SIGRTMIN = %d, SIGRTMAX = %d\n", (int)SIGRTMIN, (int)SIGRTMAX);

  if((pid = fork()) == 0) {
	/* child: block three realtime signals */
	sigemptyset(&newset);
	sigaddset(&newset, SIGRTMAX);
	sigaddset(&newset, SIGRTMAX-1);
	sigaddset(&newset, SIGRTMAX-2);
	sigprocmask(SIG_BLOCK, &newset, NULL);

	/* establish signal hanlder with SA_SIGINFO set */
    signal_rt(SIGRTMAX, sig_rt, &newset);
	signal_rt(SIGRTMAX-1, sig_rt, &newset);
	signal_rt(SIGRTMAX-2, sig_rt, &newset);

	sleep(6); /* let parent send all the signals */
	sigprocmask(SIG_UNBLOCK, &newset, NULL);
	sleep(3); /* let all queued signals be delivered */
	return 0;
  }

  /* parent sends nine signals to child */
  sleep(3); /*let child block all signals */
  for(i = SIGRTMAX; i >= SIGRTMAX-2; --i) { /* descend order send*/
	for(j = 3; j > 0; --j) {
	  val.sival_int = j;
	  sigqueue(pid, i, val);
	  printf("send signal %d, val = %d\n", i, j);
	}
  }
  wait(NULL);
}

static void sig_rt(int signo, siginfo_t *info, void *context) {
  printf("received signal #%d, code = %d, ival = %d\n",
	  signo, info->si_code, info->si_value.sival_int);
}

sa_sigaction_ptr signal_rt(int signo, sa_sigaction_ptr func, sigset_t *mask) {
  struct sigaction act, oldact;
  act.sa_sigaction = func;
  act.sa_mask = *mask;
  act.sa_flags = SA_SIGINFO;
  if(signo == SIGALRM)
	act.sa_flags |= SA_RESTART;
  if(sigaction(signo, &act, &oldact) < 0)
	return ((sa_sigaction_ptr)SIG_ERR);
  return oldact.sa_sigaction;
}
