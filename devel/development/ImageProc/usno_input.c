#include "imageproc.h"

#define convert M_PI/180.0

main(argc,argv)
	int argc;
	char *argv[];
{
  	char 	imagename[500],outname[50],line[500],offsetdat_in[500], 
		outdat_in[500],sqlscript[500],usnob_cat[500],nite[100];
  	int 	ccd, ccdnum, i=0,flag_new=0;
  	float 	ra, dec, raoff, decoff, rahw, dechw, ralow, rahigh, 
		declow, dechigh;

  	FILE 	*fin, *foff, *fout;

  	if(argc < 4) { 
	  printf("usno_input <offsets.dat> <images.dat> <output.sql> <usnob.cat> \n"); 
	  exit(0); 
	}

  	sprintf(offsetdat_in, "%s", argv[1]);
  	sprintf(outdat_in, "%s", argv[2]);
  	sprintf(sqlscript, "%s", argv[3]);
  	sprintf(usnob_cat, "%s", argv[4]);

	/* read in ccd offsets and half widths */
  	foff = fopen(offsetdat_in, "r");
  	fscanf(foff, "%d %f %f %f %f", &ccd, &raoff, &decoff, &rahw, &dechw);
  	fclose(foff);


	/* check to see whether we are beginning new file or appending */
	fin=fopen(sqlscript,"r");
	while (fgets(line,500,fin)!=NULL) {
	  i++;
	  if (i>4) break;
	}
	fclose(fin);
	if (i==4) flag_new=1;

	/* now create sql script */
  	fin = fopen(outdat_in, "r");
  	fout = fopen(sqlscript, "a");
  	i = 0;
  	while(fscanf(fin,"%s %f %f %d",imagename,&ra,&dec,&ccdnum) != EOF) {

      	  if(ccdnum == ccd) 
	    {
	      ra = ra + raoff/cos(dec*convert);
	      ralow = ra - rahw/cos(dec*convert) - 1.6;
	      rahigh = ra + rahw/cos(dec*convert) + 1.6;

	      dec = dec + decoff;
	      declow = dec - dechw;
	      dechigh = dec + dechw;
      	  
	      if (flag_new && i==0) 
		fprintf(fout,
			"(ra between %2.6f and %2.6f and dec between %2.6f and %2.6f)\n", 
			ralow, rahigh, declow, dechigh);
	      else 
		fprintf(fout,
			"or (ra between %2.6f and %2.6f and dec between %2.6f and %2.6f)\n", 
			ralow, rahigh, declow, dechigh);
	      i++;
	    }
	  else
	    printf("CCD numbers in %s and %s are not the same for image %s\n", offsetdat_in, outdat_in, imagename);
    	}
  	fclose(fin);
  	fclose(fout);
	
	return(0);
}

#undef convert
