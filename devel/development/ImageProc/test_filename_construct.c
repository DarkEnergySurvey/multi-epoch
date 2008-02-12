#include "imageproc.h"


main(argc,argv)
	int argc;
	char *argv[];
{
	char	filename[1000],runiddesc[500],imagename[1000],imagetype[100],
		band[100],tilename[100],nite[100],imageclass[100];
	int	ccd_number;
	void 	filename_construct();


	if (argc<2) {
	  printf("test_filename_construct <imageclass> <runiddesc> <nite> <tilename> <imagetype> <imagename> <band> <ccd_number>\n");
	  exit(0);
	}
	sprintf(imageclass,argv[1]);
	sprintf(runiddesc,argv[2]);
	sprintf(nite,argv[3]);
	sprintf(tilename,argv[4]);
	sprintf(imagetype,argv[5]);
	sprintf(imagename,argv[6]);
	sprintf(band,argv[7]);
	if (strlen(argv[8])) sscanf(argv[8],"%d",&ccd_number);
	else ccd_number=0;
				
	filename_construct(filename,imageclass,runiddesc,nite,tilename,
	  imagetype,imagename,band,ccd_number);
	  
	printf("%s:\n",filename);
	printf("  imclass: %-6s runid: %-15s nite: %-15s imtype: %-8s\n",
	  imageclass,runiddesc,nite,imagetype);
	printf("  tilename: %-15s imname: %-20s band: %-6s ccdnum: %2d\n",
	  tilename,imagename,band,ccd_number);
	
}

