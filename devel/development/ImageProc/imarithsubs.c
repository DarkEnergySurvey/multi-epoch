/* image arithmetic subroutines */
#define VERBOSE 0

/* currently assumes image data in first extension and doesn't bother */
/* to read other extensions */

#include "imageproc.h"

void overscan(data,output,flag_quiet)
	desimage *data,*output;
	int flag_quiet;
{
	float	*vecsort0,*vecsort1;
	short	*overscan0,*overscan1;
	int	length,y,x,xmin,xmax;
	unsigned long width;
	void	shell();
		
	    /* create overscan vectors for each amp */
	    /* assume that the overscan is a set of columns of */
	    /* equal size on both sides of image */
	    length=data->biassecan[3]-data->biassecan[2]+1;
	    overscan0=(short *)calloc(length,sizeof(short));
	    overscan1=(short *)calloc(length,sizeof(short));
	    vecsort0=(float *)calloc(length,sizeof(float));
	    vecsort1=(float *)calloc(length,sizeof(float));
	    width=data->biassecan[1]-data->biassecan[0]+1;
	    for (y=0;y<length;y++) {
	      /* copy overscan for current line into sorting vector */
	      for (x=0;x<width;x++) {
	        vecsort0[x]=data->image[y*data->axes[0]+x+data->biassecan[0]-1];
	        vecsort1[x]=data->image[y*data->axes[0]+x+data->biassecbn[0]-1];
	      }
	      /* sort the vectors */
	      shell(width,vecsort0-1);
	      shell(width,vecsort1-1);
	      /* copy median into overscan vectors */
	      overscan0[y]=vecsort0[width/2];
	      overscan1[y]=vecsort1[width/2];
	    }

	  /* create a single image array that fits the trimmed data */
	  output->axes[0]=1+data->trimsecn[1]-data->trimsecn[0];
	  output->axes[1]=1+data->trimsecn[3]-data->trimsecn[2];
	  output->nfound=2;
	  output->npixels=output->axes[0]*output->axes[1];
	  output->image=(float *)calloc(output->npixels,sizeof(float));
	  output->saturateA=data->saturateA;output->saturateB=data->saturateB;
	  output->gainA=data->gainA;output->gainB=data->gainB;
	  output->rdnoiseA=data->rdnoiseA;output->rdnoiseB=data->rdnoiseB;
	  
	  /* copy the image into the new data array with overscan correction */
	  xmin=data->ampsecan[0]+data->datasecn[0]-1;
	  xmax=data->ampsecan[1]+data->datasecn[0]-1;
	  
	  for (y=data->trimsecn[2];y<=data->trimsecn[3];y++) 
	    for (x=data->trimsecn[0];x<=data->trimsecn[1];x++) {
	    if (x>=xmin && x<xmax) /* then we are in AMPSECA */   {
	      output->image[(y-data->trimsecn[2])*output->axes[0]+
		(x-data->trimsecn[0])]=data->image[(y-1)*data->axes[0]+x-1]-
		overscan0[y-data->trimsecn[2]];
	    }
	    else /* assume we are in AMPSECB */  {
	      output->image[(y-data->trimsecn[2])*output->axes[0]+
	        (x-data->trimsecn[0])]=
	        data->image[(y-1)*data->axes[0]+x-1]-overscan1[y-data->trimsecn[2]];
	    }
	  }
}


void getxyz(ra,dec,x,y,z)
	double ra,dec;  
        double *x,*y,*z;     
{
  double cd,pr=M_PI/180.0;

  ra*=pr; dec*=pr;
  
  cd=cos(dec);
  *x=cos(ra)*cd;
  *y=sin(ra)*cd;
  *z=sin(dec);
}

/* subroutine calculates the median value within an image */
void  retrievescale(image,scaleregionn,scalesort,flag_quiet,scalefactor,
	mode,fwhm)
	desimage *image;
	float	scalesort[],*scalefactor,*mode,*fwhm;
	int	scaleregionn[],flag_quiet;
{
	int	i,x,y,loc,xdim,npixels,width,j,
		jmax,jplus,jminus,num;
	float	*histx,*histy,ymax,fraction;

	i=0;
	xdim=image->axes[0];
	npixels=image->npixels;
	/* copy good image values into sorting vector */
	for (y=scaleregionn[2]-1;y<scaleregionn[3]-1;y++) 
	  for (x=scaleregionn[0]-1;x<scaleregionn[1]-1;x++) {
	    loc=x+y*xdim;
	    if (loc>=0 && loc<npixels) {
	      if (!(image->mask[loc])) { /* of pixel not masked */
	        scalesort[i]=image->image[loc];
	        i++;
	      }
	    }
	}
	if (i<100) {
	  printf("  **** Useable scale region [%d:%d,%d:%d] too small for %s\n",
	    scaleregionn[0],scaleregionn[1],scaleregionn[2],scaleregionn[3],
	    image->name);
	  *scalefactor=0.0; /* mark image as unuseable */
	}
	else {
	  /* sort list */
	  shell(i,scalesort-1);
	  /* grab the median */
	  if (i%2) *scalefactor=scalesort[i/2];
	  else *scalefactor=0.5*(scalesort[i/2]+scalesort[i/2-1]);
	  /* examine histogram */
	  width=1000;
	  num=i/width/2;
	  histx=(float *)calloc(num,sizeof(float));
	  histy=(float *)calloc(num,sizeof(float));
	  ymax=0.0;jmax=jplus=jminus=0;
	  for (j=num;j>0;j--) {
	    loc=width+(j-1)*2*width;
	    histx[j-1]=(scalesort[loc+width]+scalesort[loc-width])*0.5;
	    histy[j-1]=2.0*width/(scalesort[loc+width]-scalesort[loc-width]);
	    if (histy[j-1]>ymax) {
	      ymax=histy[j-1];
	      jmax=j-1;
	    }
	  }
	  for (j=jmax;j<num;j++) 
	    if (histy[j]<0.5*ymax) {
	      jplus=j;
	      break;
	    }
	  for (j=jmax;j>=0;j--) 
	    if (histy[j]<0.5*ymax) {
	      jminus=j;
	      break;
	    }
	    
	  *mode=histx[jmax];
	  *fwhm=histx[jplus]-histx[jminus];

	  if (!flag_quiet) {
	    printf("    Image: %s\n",image->name);
	    fraction= (float)i/(float)((scaleregionn[1]-scaleregionn[0])*
	      (scaleregionn[3]-scaleregionn[2]));
	    printf("      [%d:%d,%d:%d] w/ %.5f of pixels useful.\n",
	      scaleregionn[0],scaleregionn[1],scaleregionn[2],scaleregionn[3],
	      fraction);
	    printf("      Scalefactor %.1f Mode %.1f FWHM %.1f\n",
	      *scalefactor,*mode,*fwhm);		
	  }

	  free(histx);free(histy);
	}
}

