/* convert the list output from scampmkcat to "standard" format */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

main(argc,argv)
        int argc;
        char *argv[];
{
  char listin[1000],listout[1000],imagename[1000],band[2],impath[1000];
  int ccd;
  FILE *fin, *fout;

  if(argc < 2) {
    printf("Usage: %s <inlist> <outlist>\n",argv[0]);
    exit(0);
  }

  sprintf(listin,"%s",argv[1]);
  sprintf(listout,"%s",argv[1]);
  
  fin=fopen(listin,"r");
  fout=fopen(listout,"w");

  while(fscanf(fin,"%s %s %d %s",imagename, band, &ccd, impath)!=EOF) 
    fprintf(fout,"%s_im.fits\n",impath);

  fclose(fin); fclose(fout);
}
