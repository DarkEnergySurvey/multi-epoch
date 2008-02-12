#include <stdlib.h>
#include <stdio.h>
#include <string.h>

main(argc,argv)
     int argc;
     char *argv[];
{
  char logfile[1024],line[1000],comment[2048];
  char catname[1024],temp[20],temp1[20],temp2[20];
  int i,nstar,ntem,flag_chi2=0;
  float chi2_1,chi2_2,chi2_in,sig1,sig2;
  FILE *pip;

  if(argc<2) {
    printf("Usage: %s <astrometry.log>\n",argv[0]);
    printf("      Option:\n");
    printf("             -chi2 <#>\n");    
    exit(0);
  }

  sprintf(logfile,"%s",argv[1]);
  for(i=2;i<argc;i++) {
    if (!strcmp(argv[i],"-chi2")) {
      flag_chi2=1;
      sscanf(argv[i+1],"%f",&chi2_in);
    }
  }

  sprintf(comment,"grep --regexp='Examining Catalog' --after-context=4 --regexp='Astrometric stats (external)' %s",logfile);
  
  pip=popen(comment,"r");
  while (fgets(line,1000,pip)!=NULL) {
 
    for (i=strlen(line);i>0;i--) {
      if (!strncmp(&(line[i]),"Examining",9)) 
	sscanf(line,"%s %s %s %s",temp,temp,temp,catname);
    }

    if(!strncmp(&(line[i]),"Group",5)) {
      sscanf(line,"%s %s %s %s %f %d %s %s %f ",temp,temp,temp1,temp2,&chi2_1,&nstar,temp,temp,&chi2_2);

      for(i=strlen(temp1);i>0;i--) 
	if (!strncmp(&(temp1[i]),"'",1)) 
	  temp1[i]=0;
      sscanf(temp1,"%f",&sig1);
      for(i=strlen(temp2);i>0;i--) 
	if (!strncmp(&(temp2[i]),"'",1)) 
	  temp2[i]=0;
      sscanf(temp2,"%f",&sig2);

      if(flag_chi2) {
	if(chi2_2 > chi2_in) {
	  if(strlen(catname)>32) printf("%s\t%d\t%2.1f\t%2.1f\t%2.3f\t%2.3f\n",catname,nstar,chi2_1,chi2_2,sig1,sig2);
	  else printf("%s\t\t%d\t%2.1f\t%2.1f\t%2.3f\t%2.3f\n",catname,nstar,chi2_1,chi2_2,sig1,sig2);
	}
      }
      else {
	if(strlen(catname)>32) printf("%s\t%d\t%2.1f\t%2.1f\t%2.3f\t%2.3f\n",catname,nstar,chi2_1,chi2_2,sig1,sig2);
	else printf("%s\t\t%d\t%2.1f\t%2.1f\t%2.3f\t%2.3f\n",catname,nstar,chi2_1,chi2_2,sig1,sig2);
      }
    }
  }
  pclose(pip);
}
