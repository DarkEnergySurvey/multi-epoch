#include "imageproc.h"

main(argc,argv)
	int argc;
	char *argv[];
{
	char	name[200],command[200],
		bigcommand[100000];
	FILE	*inp,*out;
	int	j,len,loc;

	if (argc<2) {
	  printf("makedirs <file>\n");
	  exit(0);
	}

	/* read directory list and create all the directories */
	inp=fopen(argv[1],"r");
	if (inp==NULL) {
	  printf("  makedirs:  File not found %s\n",argv[1]);
	  exit(0);
	}
	while (fscanf(inp,"%s",name)!=EOF) {
	  /*strip out the directory name */ 
	  len=strlen(name);
	  for (j=len;j>0;j--) 
	    if (!strncmp(&(name[j]),"/",1)) {
	      name[j]=0;
	      break;
	    }
	  loc=strlen(bigcommand);
	  sprintf(bigcommand+loc,"mkdir -p %s;\n",name);
	}
	fclose(inp);

	printf("%s",bigcommand);

	/* create all the directories */
	system(bigcommand);

	return(0);
}
