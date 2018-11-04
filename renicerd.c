/*
Copyright (c) 2005, DrumFire
All rights reserved.

Redistribution and use in source and binary forms, with 
or without modification, are permitted provided that the 
following conditions are met:

    * Redistributions of source code must retain the above 
      copyright notice, this list of conditions and the following
      disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY 
WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <kvm.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <sysexits.h>
#include <signal.h>
#include <ctype.h>
#include <inttypes.h>

#define		_PATH_LOGPID	"/var/run/renicerd.pid"

#define	fxtofl(fixpt)	((double)(fixpt) / fscale)

char	Copyright[]="Copyright (c) 2005, DrumFire";

void    usage();
void	exitprocess( int signo );

/* - Declare pointer of kvm_t struct - */

kvm_t *mykernel;

int    cnt;

struct record {
        char processname[21];
        unsigned int  cpu;
	int	nice;
        struct record *pun;
};

struct record   *parse_cfg();

/* - Declare pointer to record structure - */

struct record *puntatore,*puntatore2;

struct sigaction   sa_new,sa_old;

char   *configfile;

int
main (int argc, char *argv[] ) { 

/* - Variables - */

FILE *fp;
fixpt_t	ccpu;
size_t oldlen;
time_t	oldtime;
int     ch,fflag,sstime;
unsigned int    sleeptime;
extern char *optarg;
extern int opterr, optind, optopt;
struct kinfo_proc *myproc;
struct stat	filestat;
const char errstr[_POSIX2_LINE_MAX];
unsigned int reniced,i,fscale,percentuale;

reniced=0;
fflag=0;
i=0;
sstime=0;

err_set_exit(exitprocess);

	while((ch = getopt(argc,argv, "f:s:")) != -1)
                switch (ch) {
                case 'f':
                        fflag=1;
                        if ( asprintf(&configfile,"%s",optarg) != -1 ) {
                                if ( (access(configfile,R_OK)) == -1 ) {
                                        fprintf(stderr,"%s: %s\n",configfile,strerror(errno));
                                        exit(1);
                                }
                        } else {
                                printf("Error");
                                exit(1);
                        }
                        break;
                case 's':
			sstime=1;
                        while ( optarg[i] != '\0') {
                                if ( !isdigit(optarg[i]) ) {
                                        printf("Error, this is not a digital number\n");
                                        usage();
                                        exit(1);
                                }
                        i++;
                        }
                        sleeptime=(int)strtoimax(optarg, (char **)NULL, 10);
                        if ( !( sleeptime >= 0 && sleeptime <= 65535 ) ) {
                                printf("Number out of range\n");
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

if(!fflag)
	if( asprintf(&configfile,"/usr/local/etc/renicerd.conf") == -1 ) 
		err(1, "out of memory");

if(!sstime)
	sleeptime=30;

//if (asprintf(&PidFile, _PATH_LOGPID) == -1)
//	err(1, "out of memory");

/* - Check if PID file is present - */

if( (access(_PATH_LOGPID,R_OK)) == 0 ) {
	printf("/var/run/renicerd.pid locked! Maybe another istance of renicerd is running?\n");
	exit(1);
}

/* - Parse configuration file and exit if pointer of a structure is NULL - */

if((puntatore=parse_cfg()) == NULL) 
	err(EX_SOFTWARE,"Error in config file");

if( stat(configfile,&filestat) == -1 ) {
	fprintf(stderr,"%s: stat(2)\n",strerror(errno));
	exit(1);
}


/* - Open Kernel Virtual Memory - */

if((mykernel=(kvm_t *) kvm_open(NULL,NULL,NULL,O_RDONLY,errstr)) == NULL)
		err(1,"Error in kvm_open:");

oldlen=sizeof(ccpu);

/* - Retrieve value of kern.fscale sysctl variables - */

if (sysctlbyname("kern.fscale", &fscale, &oldlen, NULL, 0) == -1)
return (1);

/* - Fork this process to run in background - */

if (daemon(0, 0) == -1)
	err(EX_OSERR, "unable to fork");

/* - Set PID File - */

fp = fopen(_PATH_LOGPID, "w");
	if (fp != NULL) {
       		fprintf(fp, "%d\n", getpid());
       		(void)fclose(fp);
	} else {
	 	fprintf(stderr,"Error creating Pid file: %s: \n",strerror(errno));
	        exit(1);
	}	

/* - Save the original last modified value to check later - */

oldtime=filestat.st_mtime;

/* - Catch SIGTERM signal, to manage exitprocess function - */

sa_new.sa_handler = exitprocess;
sigemptyset(&sa_new.sa_mask); /* Clear mask */
sa_new.sa_flags = 0; /* No special flags */
sigaction(SIGTERM,&sa_new,&sa_old);

/* - Save in puntatore2 the original pointer of structure - */

puntatore2=puntatore;

   while(1) {

	/* - Get list of process with kvm_getprocs - */
	
	if((myproc=(struct kinfo_proc *) kvm_getprocs(mykernel,KERN_PROC_PROC,0,&cnt)) == NULL)
		err(1,"unable to get procs (kvm_getprocs)");

	/* - Check if configfile is changed from the last check. Then reload it - */

	if( stat(configfile,&filestat) == -1 ) {
        	fprintf(stderr,"%s: stat(2)\n",strerror(errno));
        	exit(1);
	}


	if ( filestat.st_mtime > oldtime ) {
	
		oldtime=filestat.st_mtime;

			puntatore=puntatore2;

        			while ( puntatore!=NULL ) {
                			puntatore2=puntatore;
                			puntatore=puntatore->pun;
                			free(puntatore2);
        			}
	
			if((puntatore=parse_cfg()) == NULL) {
				err(1,"Error in config file");
				exit(1);
			}
		puntatore2=puntatore;
	        syslog(LOG_CONS,"Config file changed! Reload...");
	}


	for (i=0;i<cnt;i++) {

	   puntatore=puntatore2;
		
		while ( puntatore != NULL ) {
	  
			/* - Check if process name of each element is the same of process - */
			if(strcmp(puntatore->processname,myproc[i].ki_comm) == 0) {

			    percentuale=100.0 * fxtofl(myproc[i].ki_pctcpu);

			      if((percentuale > puntatore->cpu) && (myproc[i].ki_nice != puntatore->nice)) {
				
				setpriority(PRIO_PROCESS, (int) myproc[i].ki_pid , puntatore->nice);
        	                syslog(LOG_CONS,"Process named %s(PID: %d) reniced!",myproc[i].ki_comm,myproc[i].ki_pid);
				reniced=1;
		              }		
		        }
     		 /* - If process is reniced, than set puntatore=NULL and go to next process - */ 
		   if(reniced != 1) {
		  	puntatore=puntatore->pun;
                   } else {
			puntatore=NULL;
			reniced=0;
		   }		                
 
	       } /* - While close - */

	} /* - For close - */

    /* - Wait sleep seconds to check processes again - */
    sleep(sleeptime);
   }


return 0;
} 

void
exitprocess(int sig ) {

	remove(_PATH_LOGPID);

	
	if ( puntatore2 != NULL) {
	     puntatore=puntatore2;

/* -    Free memory     - */ 
        	while ( puntatore!=NULL ) {
                	puntatore2=puntatore;
                	puntatore=puntatore->pun;
                	free(puntatore2);
        	}
	}
	//free(PidFile);

	/* - Close Kernel Virtual Memory - */
	if ( mykernel != NULL) 
		kvm_close(mykernel);
        syslog(LOG_CONS,"Exiting...");
	sigaction(SIGTERM,&sa_old,0);
	exit(0);
}

struct
record *parse_cfg() {

        struct record *p,*punt;
        FILE *fp;
        char stringa4[100],process[21];
        unsigned int cpu;
	int	nice;
        if ( (p = (struct record *) malloc(sizeof(struct record))) == NULL) {
		fprintf(stderr,"Malloc error %s",strerror(errno));
		exit(1);
	}
        punt = p ;


/*	- Set configfile name in *configfile -*/

        if((fp=fopen(configfile,"r")) == NULL) {
		err(1,configfile,NULL);
		exit(1);
	}

        while (!feof(fp) ) {

        fgets(stringa4,100,fp);
          if (strncmp(stringa4,"#",1) != 0 ) {

                sscanf(stringa4,"%20s %d %d",process,&cpu,&nice);

                        strlcpy(punt->processname,process,20);
                        punt->cpu=cpu;
                        
			if(( nice > 20) || ( nice < -20)) {
			   
				printf("Nice level of %s is < -20 or > 20. Check Configuration. Exit.\n",process);
				exit(1);
			}
				punt->nice=nice;
                        if(!feof(fp)) {

                                if ( (punt->pun=(struct record *)malloc(sizeof(struct record))) == NULL ) {
				 	fprintf(stderr,"Malloc error %s",strerror(errno));	
					exit(1);
				}
                                punt=punt->pun;
                        } else {
                                punt->pun=NULL;
                        }
          }
        }

fclose(fp);
return(p);
}

void
usage()
{
        printf("Renicerd 1.0: An autorenicer Daemon\n");
        printf("Use: renicerd -f [alternateconfig] -s [sleeptime]\n");
}

