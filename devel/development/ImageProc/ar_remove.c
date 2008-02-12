#include "imageproc.h"

#define REMAP 4
#define REDUCED 3

#define BANDMAX 4
#define IMTYPEMAX 6
#define CCDMAX 8

main(argc,argv)
	int argc;
	char *argv[];
{
	char	tagimtype[IMTYPEMAX][100]={"zero","dome flat","object","reduced",
		"remap","catalog"},imname[100],archive[500],
		runid[500],trash[5000],file[500],
		tagband[BANDMAX][10]={"g","r","i","z"},
		nite[100],command[500],list_name[500],
		query[25]="count(*)";
	int	i,files[IMTYPEMAX][BANDMAX+1][CCDMAX],
		objects[IMTYPEMAX][BANDMAX+1][CCDMAX],
		remapobjects=0,reducedobjects=0,
		band,imtype,ccd,flag_quiet=0,ct=0,number;
	FILE	*out,*pip;

	if (argc<2) {
	  printf("ar_remove <runid>\n");
	  printf("  -archive <archive directory>\n");
	  exit(0);
	}

	sprintf(runid,"%s",argv[1]);

	/* place default archive location */
	sprintf(archive,"/Archive");
	for (i=2;i<argc;i++) {
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	  if (!strcmp(argv[i],"-archive")) {
	    i++;
	    sprintf(archive,"%s",argv[i]);
	  }
	}

	/* call stored procedure to remove data */
	sprintf(file,"ar_remove_%s.sql",runid);
	out=fopen(file,"w");
	fprintf(out,"exec DelCoadd(\'%s\');\n",runid);
	fclose(out);

        sprintf(command,"sqlplus -S pipeline/dc01user@charon.ncsa.uiuc.edu/des < %s\n",file);
	system(command);
	

	/* now remove the actual files on the current archive node */
	sprintf(command,"rm %s;rm -rf %s/red/%s",file,archive,runid);
	system(command);
	printf("  Removed the directory %s/red/%s\n",archive,runid);
	printf("    as well as all associated objects and images in the db\n");
}


#undef REMAP
#undef REDUCED


#undef BANDMAX
#undef IMTYPEMAX
#undef CCDMAX
