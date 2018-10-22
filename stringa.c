#include <stdio.h>

int main() {

	char *stringa,*stringa3,*stringa2,*stringa4;
	FILE *fp;
	int cnt=0;

	stringa=(char *) malloc(1000);
	stringa2=(char *) malloc(1000);
	stringa3=(char *) malloc(1000);
	stringa4=(char *) malloc(1000);

	fp=fopen("renicerd.conf","r");

        while (!feof(fp) ) {
	
	fgets(stringa4,100,fp);
	  if (strncmp(stringa4,"#",1) != 0 ) {
 
		sscanf(stringa4,"%10s %s %s",stringa,stringa2,stringa3);
		cnt++;	
			if(!feof(fp)) 
        		   printf("%s %s %s\n",stringa,stringa2,stringa3);
				
	  }
	}

printf("Numero di righe nel file di configurazione %d\n",cnt);
free(stringa);
free(stringa2);
free(stringa3);
free(stringa4);

fclose(fp);
}

