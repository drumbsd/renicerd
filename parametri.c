#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

void		usage();

int
main(int argc, char *argv[]) {

	long	num;
	int	i,ch,fflag;
	unsigned int	sleep;
	extern char *optarg;
	extern int opterr, optind, optopt;
	char	*nomefile;
	
	fflag=0;
	i=0;

	while((ch = getopt(argc,argv, "f:s:")) != -1)
		switch (ch) {
		case 'f':
			fflag=1;
			if ( asprintf(&nomefile,optarg) != -1 ) {
				if ( (access(nomefile,R_OK)) == -1 ) {	
					fprintf(stderr,"%s: %s\n",nomefile,strerror(errno));
					exit(1);
				}
			} else {	
				printf("Errore");
				exit(1);
			}
			break;
		case 's':
			sleep=1;
			//if ( isdigit(optarg) )
			//	printf("%s\n",optarg);
			while ( optarg[i] != '\0') {
				if ( !isdigit(optarg[i]) ) {
					printf("Error, non e' un numero\n");
					usage();
					exit(1);	
				}
			i++;
			}
			sleep=(int)strtoimax(optarg, (char **)NULL, 10);
			if ( !( sleep >= 0 && sleep <= 65535 ) ) {
				printf("Numero fuori posto\n");
				exit(1);
			}
			break;
		case '?':
			 usage();
			 exit(1);	
		default:	
			 break;
		}
	argc -= optind;
	argv += optind;	

if(fflag)
	free(nomefile);
return 0;
}

void
usage() 
{
	printf("Renicerd 1.0: An autorenicer Daemon\n");
	printf("Use: renicerd -f [alternateconfig] -s [sleeptime]\n");
}
