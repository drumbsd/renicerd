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

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <kvm.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/syslog.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

#define		SLEEP		30
#define		_PATH_LOGPID	"/var/run/renicerd.pid"

#ifdef __OpenBSD__
#define		NICEOPENBSDCALC -20
#endif

#ifndef __OpenBSD__
#define		NICEOPENBSDCALC 0
#endif

#define	fxtofl(fixpt)	((double)(fixpt) / fscale)

char	Copyright[]="Copyright (c) 2005, DrumFire";

struct record {

        char processname[20];
        int  cpu;
        int  nice;
        struct record *pun;
};

void		exitprocess( int signo );

struct record   *parse_cfg();

/* - Declare pointer to record structure - */

struct record *puntatore,*puntatore2;

/* - Declare pointer of kinfo_proc and kvm_t struct - */

kvm_t *mykernel;
int    *cnt;

char   static *configfile="/usr/local/etc/renicerd.conf";

char      *PidFile;
int
main () { 

struct kinfo_proc *myproc;
struct stat	filestat;
time_t	oldtime;

/* - Variables - */
FILE *fp;
const char errstr[_POSIX2_LINE_MAX];
int mib[2],reniced=0,i,fscale,percentuale;
size_t siz;

/* - Check if PID file is present - */

if (asprintf(&PidFile,_PATH_LOGPID) == - 1)
	err(1, "out of memory");

if( (access(PidFile,R_OK)) == 0 ) {
	printf("/var/run/renicerd.pid locked! Maybe another istance of renicerd is running?\n");
	exit(1);
}

cnt=(int *) malloc(sizeof(int));

/* - Parse configuration file and exit if pointer of a structure is NULL - */

if((puntatore=parse_cfg()) == NULL) {
	err(1,"Error in config file");
	exit(1);
}

if( stat(configfile,&filestat) == -1 ) {
	fprintf(stderr,"%s: stat(2)\n",strerror(errno));
	exit(1);
}

/* - Save the original last modified value to check later - */

oldtime=filestat.st_mtime;

/* - Open Kernel Virtual Memory - */

if((mykernel=(kvm_t *) kvm_open(NULL,NULL,NULL,O_RDONLY,errstr)) == NULL)
		err(1,"Error in kvm_open. Exit...");

//oldlen=sizeof(ccpu);

siz=sizeof(fscale);

/* - Retrieve value of kern.fscale sysctl variables - */
/*
if (sysctlbyname("kern.fscale", &fscale, &oldlen, NULL, 0) == -1)
return (1);
*/
mib[0] = CTL_KERN;
mib[1] = KERN_FSCALE;
if (sysctl(mib, 2, &fscale, &siz, NULL, 0) < 0) {
	warnx("fscale: failed to get kern.fscale");
}
/* - Fork this process to run in background - */

if (daemon(0, 0) == -1)
	err(1, "unable to fork");

/* - Set PID File - */

fp = fopen(PidFile, "w");
	if (fp != NULL) {
       		fprintf(fp, "%d\n", getpid());
       		(void)fclose(fp);
	} else {
                fprintf(stderr,"Error creating Pid file: %s: \n",strerror(errno));
                exit(1);
        }


/* - Catch SIGTERM signal, to manage exitprocess function - */

signal(SIGTERM, exitprocess);

puntatore2=puntatore;

   while(1) {

	/* - Get list of process with kvm_getprocs - */
	
	if((myproc=(struct kinfo_proc *) kvm_getprocs(mykernel,KERN_PROC_ALL,0,sizeof(struct kinfo_proc), cnt)) == NULL)
		err(1,"unable to get procs (kvm_getprocs)");

	/* - Check if configfile is changed from the last check. Then reload it - */

	if( stat(configfile,&filestat) == -1 ) {
        	fprintf(stderr,"%s: stat(2)\n",strerror(errno));
        	exit(1);
	}

	if ( filestat.st_mtime > oldtime ) {
	
		oldtime=filestat.st_mtime;

			/* - Free Memory - */
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

	for (i=0;i<*cnt;i++) {

	   puntatore=puntatore2;

		while ( puntatore != NULL ) {
	  
			/* - Check if process name of each element is the same of process - */
			if(strcmp(puntatore->processname,myproc[i].p_comm) == 0) {

			    percentuale=100.0 * fxtofl(myproc[i].p_pctcpu);

			      if((percentuale > puntatore->cpu) && (myproc[i].p_nice+NICEOPENBSDCALC != puntatore->nice)) {
				
				setpriority(PRIO_PROCESS, (int) myproc[i].p_pid , puntatore->nice);
        	                syslog(LOG_CONS,"Process named %s(PID: %d) reniced!",myproc[i].p_comm,myproc[i].p_pid);
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

    /* - Wait SLEEP seconds to check processes again - */
    sleep(SLEEP);
   }


return 0;
} 

void
exitprocess(int sig ) {

	remove(PidFile);
        syslog(LOG_CONS,"Exiting...");

	puntatore=puntatore2;

/* -    Free memory     - */ 
        while ( puntatore!=NULL ) {
                puntatore2=puntatore;
                puntatore=puntatore->pun;
                free(puntatore2);
        }
	free(cnt);

	/* - Close Kernel Virtual Memory - */
	kvm_close(mykernel);
	exit(0);
}

struct
record *parse_cfg() {

        struct record *p,*punt;
        FILE *fp;
        char *stringa4,*process;
        int *cpu,*nice;

        p = (struct record *) malloc(sizeof(struct record));
        punt = p ;

        stringa4=(char *) malloc(1000);
        process=(char *) malloc(20);
        cpu=(int *) malloc(sizeof(int));
        nice=(int *) malloc(sizeof(int));

/*	- Set configfile name in *configfile -*/

        if((fp=fopen(configfile,"r")) == NULL) {
		err(1,configfile,NULL);
		exit(1);
	}

        while (!feof(fp) ) {

        fgets(stringa4,100,fp);
          if (strncmp(stringa4,"#",1) != 0 ) {

                sscanf(stringa4,"%s %d %d",process,cpu,nice);

                        strlcpy(punt->processname,process,20);
                        punt->cpu=*cpu;
                        
			if(( *nice > 20) || ( *nice < -20)) {
			   
				printf("Nice level of %s is < -20 or > 20. Check Configuration. Exit.\n",process);
				exit(1);
			}
			
				punt->nice=*nice;
                        if(!feof(fp)) {

                                punt->pun=(struct record *)malloc(sizeof(struct record));
                                punt=punt->pun;
                        } else {
                                punt->pun=NULL;
                        }
          }
        }

free(stringa4);
free(process);
free(cpu);
free(nice);
fclose(fp);
return(p);
}

