#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void 	exitprocess	(int signo);

int main () {

	struct sigaction	sa_new,sa_old;

	sa_new.sa_handler = exitprocess;	
	sigemptyset(&sa_new.sa_mask); /* Clear mask */
 	sa_new.sa_flags = 0; /* No special flags */
 	sigaction(SIGTERM,&sa_new,&sa_old);

	while(1);	
}

void exitprocess(int signo) {

	printf("esco");
	exit(0);
}
	
