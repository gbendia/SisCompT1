#include <sys/sem.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <signal.h>

int main (void)
{
	int pid;
	printf("Pid: %d\n",getpid());
	pid = fork();
	if (pid == 0)
	{	
		printf("Pid filho: %d\n", getpid());
		printf("Parei 1\n");
		kill(getpid(), SIGSTOP);
		printf("Parei 2\n");
		kill(getpid(), SIGSTOP);
		printf("Continuei\n");
		
	}
	else
	{
		kill(pid, SIGCONT);
		sleep(5);
		kill(pid, SIGCONT);
		waitpid(-1,0,0);
		printf("Pid pai: %d\n", getpid());
	}
	return 0;
}
