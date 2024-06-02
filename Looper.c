#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <signal.h>
#include <string.h>


void handler(int sig)
{
	printf("\nRecieved Signal : %s\n", strsignal(sig));
	if (sig == SIGTSTP)
	{
		signal(SIGTSTP, SIG_DFL);
		signal(SIGCONT, handler); //Task 0b
	}
	else if (sig == SIGCONT)
	{
		signal(SIGCONT, SIG_DFL);
		signal(SIGTSTP, handler); //Task 0b
	}
	signal(sig, SIG_DFL);
	raise(sig);
}

int main(int argc, char **argv)
{
	printf("Starting the program\n");
	signal(SIGINT, handler);
	signal(SIGTSTP, handler);
	signal(SIGCONT, handler);

	while (1)
	{
		sleep(1);
	}

	return 0;
}

// SIGCONT would change the state from suspended to runnable
// SIGINT would change the state from running/runnable to interrupted/killed
// SIGTSTP would change the state from running/runnable to suspended