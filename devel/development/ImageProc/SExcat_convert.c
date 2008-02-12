/* Does simple conversion of SExtractor catalog output to Object table format
*/
#include "imageproc.h"

main(argc,argv)
	int argc;
	char *argv[];
{


	int	i;


	/* define and initialize flags */
	int	flag_quiet=0,flag_list=0,imnum=1,im;
	char	imagename[200];
	FILE	*inp,*out,*cat,*images;
	long	longi,j,k;
	long	longnull=0,*flags,*objid;
	float	*x,*y,*magauto,*magerrauto,*kronradius,*magaper5,
		*magaper10,*reddening,*fwhm,*classstar,*a,*b,*theta,
		*cxx,*cyy,*cxy,floatnull=0;
	double	*ra,*dec,doublenull=0.0;
	int	anynull;
	int	ccdnum;
	char	name[200],line[1000],band[10],tmpname[200];

	if (argc<3) {
	  printf("SExcat_convert <input catalog or file list> <output file> <options>\n");
	  printf("  -quiet\n");
	  exit(0);
	}
	
	/* copy input image name if FITS file*/
	if (!strncmp(&(argv[1][strlen(argv[1])-4]),".cat",4))  {
	  sprintf(name,"%s",argv[1]);
	  imnum=1;
	}
	else { /* expect file containing list of catalogs */
	  imnum=0;flag_list=1;
	  inp=fopen(argv[1],"r");
	  while (fscanf(inp,"%s",imagename)!=EOF) {
	    imnum++;
	    if (strncmp(&(imagename[strlen(imagename)-4]),".cat",4)) {
	      printf("  ** File must contain list of SExtractor catalogs **\n");
	      exit(0);
	    }
	  }
	  fclose(inp);
	  if (!flag_quiet) printf("  Input list %s contains %d SExtractor catalogs\n",argv[1],imnum);
	  /* reopen file for processing */
	  inp=fopen(argv[1],"r");
	}
	
	/* process command line */
	for (i=2;i<argc;i++) {
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	}
		
   	sprintf(tmpname,"%s_im",argv[2]);
	if (!flag_quiet) printf("  Opening output catalog table %s and output image table %s\n",
	  argv[2],tmpname);
	/* open the output file */
	out=fopen(argv[2],"w");
	images=fopen(tmpname,"w");


	if (!flag_quiet) printf("\n");

	
	/* now cycle through input images to process them */
	for (im=0;im<imnum;im++) {

	  /* get next image name */
	  if (flag_list) fscanf(inp,"%s",name);
	
	  if (!flag_quiet) printf("  Opening %s  ",name);
	  
	  sprintf(tmpname,"%s",name);tmpname[strlen(name)-4]=0;
	  sscanf(&(tmpname[strlen(tmpname)-2]),"%d",&ccdnum);
	  tmpname[strlen(tmpname)-3]=0;
	  sscanf(&(tmpname[strlen(tmpname)-1]),"%s",band);
	  
	  fprintf(images,"%3d %02d %1s %s\n",im+1,ccdnum,band,name);
	  if (!flag_quiet) printf("imnum %4d   ccdnum %02d     band %1s\n",im+1,ccdnum,band);
	  

	  /* open ASCII catalog */
	  cat=fopen(name,"r");
	  while (fgets(line,1000,cat)!=NULL) {
	    if (strncmp(line,"#",1)) fprintf(out,"2000.0 %1s %4d 25.0000 00.0000 %s",band,im+1,line);  
	  }
	  fclose(cat);

	  if (!flag_quiet) printf("  Processing of file %d of %d complete\n\n",im+1,imnum);
	} /* end of image processing cycle with variable im */

	if (flag_list) fclose(inp);
	fclose(out);fclose(images);
	if (!flag_quiet) printf("  Closed ascii catalog file\n");
}

