#include "imageproc.h"

#define OVERLAP 0.00	/* overlap size in arcmin */
/* NOTE:  overlap will be difference between 34 arcmin and tile size,
/*	  which is something like 35.5 arcmin for 0.26 arcsec X 8192 pix */
/*#define XSIZE 34.0	/* tile size in arcmin */
/*#define YSIZE 34.0	/* tile size in arcmin */
#define XSIZE 43.0	/* tile size in arcmin */
#define YSIZE 43.0	/* tile size in arcmin */

#define VERBOSE 0

main(argc,argv)
	int argc;
	char *argv[];
{
	float	ras,decs,rasize,decsize,racurrent,deccurrent,
		epoch,pixelsize;
	double	ra,dec,decconvert(),raconvert();
	int	nx,ny,rah,ram,decd,decm,i,j,npix_ra,npix_dec;
	char	rastring[200],decstring[200],name[200],tilename[200],
		project[200],prefix[200];
	void	mkdecstring(),mkrastring();
	void	exit();
	

	if (argc<10) {
	  printf("mosaictile <project> <prefix> <Nx> <Ny> <RA: hh:mm:ss> <DEC dd:mm:ss> <epoch> <pixelsize:arcsec> <npix_ra> <npix_dec>\n");
	  exit(0);
	}
	sprintf(project,"%s",argv[1]);
	sprintf(prefix,"%s",argv[2]);
	sscanf(argv[3],"%d",&nx);
	sscanf(argv[4],"%d",&ny);
	sscanf(argv[5],"%s",rastring);
	sscanf(argv[6],"%s",decstring);
	sscanf(argv[7],"%f",&epoch);
	sscanf(argv[8],"%f",&pixelsize);
	sscanf(argv[9],"%d",&npix_ra);
	sscanf(argv[10],"%d",&npix_dec);
	
	ra=raconvert(rastring);
	dec=decconvert(decstring);
	
	printf("****  Constructing a %dX%d mosaic of pointings centered around\n",nx,ny);
	printf("****  (RA,Dec)=(%8.4f,%8.4f) \n",ra,dec);
	  
	decsize=(YSIZE-OVERLAP)/60.0;
	rasize=(XSIZE-OVERLAP)/60.0;
	
	
	printf("******** Co-addition Tiles *******\n");
	for (i=0;i<ny;i++) {
	  deccurrent=dec-decsize*((float)ny/2.0-0.5-(float)i);
	  for (j=0;j<nx;j++) {
	    /* get the pointing positions with 0 offset for the field name */
	    racurrent=ra-(rasize*((float)nx/2.0-0.5-(float)j))
	      /cos(deccurrent*M_PI/180.0);
	    if (racurrent>360.0) racurrent-=360.0;
	    if (racurrent<0.0) racurrent+=360.0;
	    mkdecstring(deccurrent,decstring,&decd,&decm,&decs);
	    mkrastring(racurrent,rastring,&rah,&ram,&ras);
	    if (decd<0) sprintf(tilename,"%s%02d%02d%03d%02d",prefix,rah,ram,decd,decm);
	    else sprintf(tilename,"%s%02d%02d+%02d%02d",prefix,rah,ram,decd,decm);
	    printf("INSERT INTO coaddtile VALUES (coaddtile_seq.NEXTVAL, '%s', '%s',",
	      project,tilename);
 	    printf(" %11.7f , %11.7f , %9.4f , %9.6f , %7d , %7d );\n",
	      racurrent,deccurrent,epoch,pixelsize,npix_ra,npix_dec);
	  }
	}
	
}
