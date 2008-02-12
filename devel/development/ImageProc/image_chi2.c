#include <stdlib.h>
#include <stdio.h>
#include <string.h>

main(argc,argv)
     int argc;
     char *argv[];
{
  char binpath[800],logfile[800],runid[600],arpath[500],nite[400],project[100];
  char command[2400],command1[2400],command2[2400];
  char line[1000],line1[1000],line2[1000],tmp[1000];
  char imagecat[500],imagename[500],rootname[500],ccdnum[50];
  int i,j,len,numlog=0,flag_hi=0,flag_lo=0,flag=0,nstar;
  float highchi2,lowchi2,chi2,temp;
  FILE *pip,*pip1,*pip2;

  if(argc<2) {
    printf("Usage: %s -binpath <<bin_path>> -arpath <<archive_path>> -runid <<runid>> -nite<<nite>> -project <<project>>\n",argv[0]);
    printf("      Option:\n");
    printf("           -highchi2 <# (chi2 higher than this value)>\n");
    printf("           -lowchi2 <# (chi2 lower than this value)>\n");
    exit(0);
  }
		    
  for (i=1;i<argc;i++) {
    if (!strcmp(argv[i],"-binpath")) sprintf(binpath,"%s",argv[i+1]);
    if (!strcmp(argv[i],"-project")) sprintf(project,"%s",argv[i+1]);
    if (!strcmp(argv[i],"-arpath")) sprintf(arpath,"%s",argv[i+1]);
    if (!strcmp(argv[i],"-runid")) sprintf(runid,"%s",argv[i+1]);
    if (!strcmp(argv[i],"-nite")) sprintf(nite,"%s",argv[i+1]);
    if (!strcmp(argv[i],"-highchi2")) {
      flag_hi=1;
      sscanf(argv[i+1],"%f",&highchi2);
    }
    if (!strcmp(argv[i],"-lowchi2")) {
      flag_lo=1;
      sscanf(argv[i+1],"%f",&lowchi2);
    }
  }

  if(flag_hi && flag_lo) flag=1;

  sprintf(command, "/bin/ls %s/%s/data/%s/log/astrometry_*.log ",arpath,runid,nite);

  pip=popen(command,"r");
  while (fgets(line,1000,pip)!=NULL) {

    sprintf(command1,"%s/QA_astrometry %s",binpath,line);    
    pip1=popen(command1,"r");
    while (fgets(line1,1000,pip1)!=NULL) {
      rootname[0]=0;
      imagename[0]=0;
      
      sscanf(line1,"%s %d %f %f %f %f",imagecat,&nstar,&temp,&chi2,&temp,&temp);
      
      len=strlen(imagecat);
      for (j=len;j>0;j--) {
	if (!strncmp(&(imagecat[j]),".cat",4)) 
	  imagecat[j]=0;
	if (!strncmp(&(imagecat[j]),"_",1)) {
	  imagecat[j]=32;
	  break;
	}
      }

      sscanf(imagecat,"%s %s",rootname,ccdnum);
      sprintf(imagename,"%s/%s_%s.fits",rootname,rootname,ccdnum);

      if(flag_hi && flag==0) {
	if(chi2 > highchi2) {
	  if(strlen(imagename)>61) printf("%s\t\t%2.1f\n",imagename,chi2);
	  else printf("%s\t\t\t%2.1f\n",imagename,chi2);

	  sprintf(command2,"%s/findtile %s %s g %s -quiet -tilename_only",binpath,imagename,nite,project);
	  pip2=popen(command2,"r");
	  while (fgets(line2,1000,pip2)!=NULL)
	    if (strncmp(line2," ** Imagename",11)) printf("%s",line2);
	  pclose(pip2);

	  sprintf(command2,"%s/findtile %s %s r %s -quiet -tilename_only",binpath,imagename,nite,project);
	  pip2=popen(command2,"r");
	  while (fgets(line2,1000,pip2)!=NULL)
	    if (strncmp(line2," ** Imagename",11)) printf("%s",line2);
	  pclose(pip2);

	  sprintf(command2,"%s/findtile %s %s i %s -quiet -tilename_only",binpath,imagename,nite,project);
	  pip2=popen(command2,"r");
	  while (fgets(line2,1000,pip2)!=NULL)
	    if (strncmp(line2," ** Imagename",11)) printf("%s",line2);
	  pclose(pip2);

	  sprintf(command2,"%s/findtile %s %s z %s -quiet -tilename_only",binpath,imagename,nite,project);
	  pip2=popen(command2,"r");
	  while (fgets(line2,1000,pip2)!=NULL)
	    if (strncmp(line2," ** Imagename",11)) printf("%s",line2);
	  pclose(pip2);
	}
      }
      else if(flag_lo && flag==0) {
	if(chi2 < lowchi2) {
	  if(strlen(imagename)>61) printf("%s\t\t%2.1f\n",imagename,chi2);
	  else printf("%s\t\t\t%2.1f\n",imagename,chi2);

	  sprintf(command2,"%s/findtile %s %s g %s -quiet -tilename_only",binpath,imagename,nite,project);
	  pip2=popen(command2,"r");
	  while (fgets(line2,1000,pip2)!=NULL)
	    if (strncmp(line2," ** Imagename",11)) printf("%s",line2);
	  pclose(pip2);

	  sprintf(command2,"%s/findtile %s %s r %s -quiet -tilename_only",binpath,imagename,nite,project);
	  pip2=popen(command2,"r");
	  while (fgets(line2,1000,pip2)!=NULL)
	    if (strncmp(line2," ** Imagename",11)) printf("%s",line2);
	  pclose(pip2);

	  sprintf(command2,"%s/findtile %s %s i %s -quiet -tilename_only",binpath,imagename,nite,project);
	  pip2=popen(command2,"r");
	  while (fgets(line2,1000,pip2)!=NULL)
	    if (strncmp(line2," ** Imagename",11)) printf("%s",line2);
	  pclose(pip2);

	  sprintf(command2,"%s/findtile %s %s z %s -quiet -tilename_only",binpath,imagename,nite,project);
	  pip2=popen(command2,"r");
	  while (fgets(line2,1000,pip2)!=NULL)
	    if (strncmp(line2," ** Imagename",11)) printf("%s",line2);
	  pclose(pip2);
	}
      }
      else if(flag) {
	if(chi2 < highchi2 && chi2 > lowchi2){
	  if(strlen(imagename)>61) printf("%s\t\t%2.1f\n",imagename,chi2);
	  else printf("%s\t\t\t%2.1f\n",imagename,chi2);

	  sprintf(command2,"%s/findtile %s %s g %s -quiet -tilename_only",binpath,imagename,nite,project);
	  pip2=popen(command2,"r");
	  while (fgets(line2,1000,pip2)!=NULL)
	    if (strncmp(line2," ** Imagename",11)) printf("%s",line2);
	  pclose(pip2);

	  sprintf(command2,"%s/findtile %s %s r %s -quiet -tilename_only",binpath,imagename,nite,project);
	  pip2=popen(command2,"r");
	  while (fgets(line2,1000,pip2)!=NULL)
	    if (strncmp(line2," ** Imagename",11)) printf("%s",line2);
	  pclose(pip2);

	  sprintf(command2,"%s/findtile %s %s i %s -quiet -tilename_only",binpath,imagename,nite,project);
	  pip2=popen(command2,"r");
	  while (fgets(line2,1000,pip2)!=NULL)
	    if (strncmp(line2," ** Imagename",11)) printf("%s",line2);
	  pclose(pip2);

	  sprintf(command2,"%s/findtile %s %s z %s -quiet -tilename_only",binpath,imagename,nite,project);
	  pip2=popen(command2,"r");
	  while (fgets(line2,1000,pip2)!=NULL)
	    if (strncmp(line2," ** Imagename",11)) printf("%s",line2);
	  pclose(pip2);
	}
      }
      else {
	if(strlen(imagename)>61) printf("%s\t\t%2.1f\n",imagename,chi2);
	else printf("%s\t\t\t%2.1f\n",imagename,chi2);

	sprintf(command2,"%s/findtile %s %s g %s -quiet -tilename_only",binpath,imagename,nite,project);
	pip2=popen(command2,"r");
	while (fgets(line2,1000,pip2)!=NULL)
	  if (strncmp(line2," ** Imagename",11)) printf("%s",line2);
	pclose(pip2);
	
	sprintf(command2,"%s/findtile %s %s r %s -quiet -tilename_only",binpath,imagename,nite,project);
	pip2=popen(command2,"r");
	while (fgets(line2,1000,pip2)!=NULL)
	  if (strncmp(line2," ** Imagename",11)) printf("%s",line2);
	pclose(pip2);
	
	sprintf(command2,"%s/findtile %s %s i %s -quiet -tilename_only",binpath,imagename,nite,project);
	pip2=popen(command2,"r");
	while (fgets(line2,1000,pip2)!=NULL)
	  if (strncmp(line2," ** Imagename",11)) printf("%s",line2);
	pclose(pip2);
	
	sprintf(command2,"%s/findtile %s %s z %s -quiet -tilename_only",binpath,imagename,nite,project);
	pip2=popen(command2,"r");
	while (fgets(line2,1000,pip2)!=NULL)
	  if (strncmp(line2," ** Imagename",11)) printf("%s",line2);
	pclose(pip2);
      }
      
    }
    pclose(pip1);
  }
  pclose(pip);
}
		    
		    
    
		    
