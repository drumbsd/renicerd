#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <ifaddrs.h>

int main() {

 struct ifaddrs *ifa,*oldifa;
 char *oldname;
 
if(getifaddrs(&ifa)) { 
        printf("No interface presents!");
	freeifaddrs(oldifa);
        exit(1);
}

oldifa=ifa;
if(asprintf(&oldname,"pippo") == -1)
	err(1, "out of memory");


while(ifa!=NULL) {
         
         if(strcmp(oldname,ifa->ifa_name)) {
            printf("%s\n",ifa->ifa_name);
            if(asprintf(&oldname,(char *) ifa->ifa_name) == -1)
			err(1, "out of memory");
         }
         ifa=ifa->ifa_next;
}
free(oldname);
freeifaddrs(oldifa);
}
