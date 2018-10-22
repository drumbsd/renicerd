#include <stdio.h>

struct record {
   
	char processname[20];
	int  cpu;
	int  nice;
   	struct record *pun;
};

struct record *parse_cfg();

int main ()  {

	struct record *puntatore,*puntatore2;

	puntatore=parse_cfg();
	puntatore2=puntatore;

	while ( puntatore->pun!=NULL ) {

		printf("Struttura lista\n");
       	 	printf("Nome Processo: %s\n",puntatore->processname);
        	printf("Valore CPU: %d\n",puntatore->cpu);
        	printf("Valore Nice: %d\n",puntatore->nice);
		puntatore=puntatore->pun;	
	}

	puntatore=puntatore2;

/* -	Free memory	- */
	while ( puntatore!=NULL ) {
		puntatore2=puntatore;
		puntatore=puntatore->pun;
		free(puntatore2);	
	}

return 0;
}

struct record *parse_cfg() {

	struct record *p,*punt;
	FILE *fp;
	char *stringa4,*process;
	int *cpu,*nice;

	p = (struct record *)malloc(sizeof(struct record));
	punt = p ;

	stringa4=malloc(1000);
	process=malloc(20);
	cpu=malloc(sizeof(int));
	nice=malloc(sizeof(int));
	
	fp=fopen("renicer.conf","r");

	while (!feof(fp) ) {
	
	fgets(stringa4,100,fp);
	  if (strncmp(stringa4,"#",1) != 0 ) {
	
		sscanf(stringa4,"%s %d %d",process,cpu,nice);
	
			strlcpy(punt->processname,process,20);
			punt->cpu=*cpu;
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


