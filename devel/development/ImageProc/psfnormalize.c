/*
** psfnormalize.c
**
** Test platform for Emmanuel Bertin's kernel solution
*/

#include "imageproc.h"
#include "fft.h"
#include "define.h"
#include "prefs.h"
#include "vignet.h"

static int verbose_flag;

main(argc,argv)
	int argc;
	char **argv;
{

  int status=0,nullval=0,naxis=0,anynull,npixels,npixelspsf,
      xsz,ysz,xszpsf,yszpsf,zszpsf=1;
  int nbigpix,i,k,c,numhdus,hdutype;
  char *filename, *psffilename, *outfilename;
  char newoutfilename[500];
  float *psfimage,*bigpsf,*fimg,*finalimg,*convimg;
  long fpixel[3]={1,1,1};
  fitsfile *fptr, *outptr;
  desimage image;

  void rd_desimage(), printerror();

  /*
  ** Handle command line arguments
  */
  while (1){

    static struct option long_options[] =
    {
      {"verbose", no_argument,       &verbose_flag, 1},
      {"quiet",   no_argument,       &verbose_flag, 0},
      {"help",    no_argument,       0, 'h'},
      {"output",  required_argument, 0, 'o'},
      {"kernel",  required_argument, 0, 'k'},
      {0,0,0,0}
    };

    int option_index=0;
    c = getopt_long_only (argc, argv, "ho:k:",long_options, &option_index);

    if (c == -1) 
      break;

    switch (c)
    {
      case 0:
        if (long_options[option_index].flag != 0) 
          break;
        printf ("option %s", long_options[option_index].name);
        if (optarg) 
          printf (" with arg %s", optarg); 
        printf ("\n"); 
        break;

      case 'h':
        printf ("\nTest bed program for convolving images with polynomial kernels\n");
        printf ("\nUsage:  psfnormalize <-output outfilename -h> -kernel kernelfile filename \n");
        break;

      case 'o':
        outfilename = optarg;
        printf ("convolved image will go to: %s\n",outfilename);
        break;

      case 'k':
        psffilename = optarg;
        printf ("kernel file: %s\n",psffilename);
        break;

      default:
        abort();

    }

  }

  /*
  ** deal with non-option command line args
  ** all that's left should be the filename to convolve
  */

  if (optind < argc)
   {
      filename = argv[optind++];
   }

  if (verbose_flag)
    printf ("filename to convolve:  %s\n",filename);

  /*
  ** End of command line parsing
  */

  /*
  ** read in the des image
  */

  sprintf(image.name,"%s",filename);
  rd_desimage(&image,READONLY,0); 
  xsz = image.axes[0];
  ysz = image.axes[1];
  npixels = xsz*ysz;

  /*
  ** Read in the psf image
  */
  if (fits_open_file(&fptr,psffilename,READONLY,&status)) printerror(status);

  if (fits_get_num_hdus(fptr,&numhdus,&status)==KEY_NO_EXIST) 
  {
              printf("  Can't get number of hdus from %s\n",psffilename);
                          printerror(status);
  }

  /* Go to the first extension in the kernel MEF file
  ** Right now, I'm not handling this intelligently, I'm expecting
  ** an MEF with one extension
  ** TODO:  Make this smarter
  */

  if (fits_movabs_hdu(fptr,2,&hdutype,&status)) printerror(status);

  if (fits_read_key_lng(fptr,"NAXIS1",&xszpsf,NULL,&status)==KEY_NO_EXIST) 
  {
     printf("  Keyword NAXIS1 not defined in %s\n",psffilename);
                          printerror(status);
  }
  if (fits_read_key_lng(fptr,"NAXIS2",&yszpsf,NULL,&status)==KEY_NO_EXIST) 
  {
     printf("  Keyword NAXIS2 not defined in %s\n",psffilename);
                          printerror(status);
  }
  if (fits_get_img_dim(fptr,&naxis,&status) ) 
  {
     printf("  Can't get number of dims for%s\n ",psffilename);
                          printerror(status);
  }

  if (verbose_flag)
    printf("There are %d hdus in %s\n",numhdus,psffilename);

  if (verbose_flag)
    printf("There are %d terms in the psf datacube \n",naxis);

  npixelspsf = xszpsf*yszpsf;

  psfimage=(float *)calloc(npixelspsf,sizeof(float));
  /*
  finalimg=(float *)calloc(npixels,sizeof(float));
  for (i=0;i<npixels;i++)
    *(finalimg++)=0.0;
   */

  /*
  ** loop through each dimension in the kernel
  ** (or term in the polynomial)
  ** Read in current term
  ** convolve image with that term
  ** add result to previous convolution
  */

  QCALLOC(bigpsf,float,npixels);
  QCALLOC(convimg,float,npixels);
  QCALLOC(finalimg,float,npixels);

  for (k=1;k<=naxis;k++){

    fpixel[2]=k;

    if (fits_read_pix(fptr,TFLOAT,fpixel,npixelspsf,nullval,psfimage,&anynull,&status))
      printerror(status);

    if (verbose_flag)
      printf("Convolving image with term %d\n",k);

    vignet_copy(psfimage,xszpsf,yszpsf,bigpsf,xsz,ysz,0,0,VIGNET_CPY);
    vignet_copy(image.image,xsz,ysz,convimg,xsz,ysz,0,0,VIGNET_CPY);

    /*
    ** Perform fft and convolution
    */

    fft_init(prefs.nthreads);
    fft_shift(bigpsf,xsz,ysz);
    fimg = fft_rtf(bigpsf,ysz,xsz);
    fft_conv(convimg,fimg,ysz,xsz);
    fft_end(prefs.nthreads);

    /*
    ** End of fft and convolution
    */

    /*
    ** add the previous term to the last
    */
    vignet_copy(convimg,xsz,ysz,finalimg,xsz,ysz,0,0,VIGNET_ADD);

    /*for (i=0;i<npixels;i++)
      *(finalimg++) += *(convimg++);
    */

    memset(convimg,0,(size_t)(xsz*ysz)*sizeof(float));

  }

  fits_close_file(fptr,&status);

  /*
  ** save the resulting file
  */
  sprintf(newoutfilename,"!%s",outfilename);

  if (fits_create_file(&outptr,newoutfilename,&status)) printerror(status);

  if (fits_create_img(outptr,FLOAT_IMG,2,image.axes,&status)) printerror(status);
  if (fits_write_img(outptr,TFLOAT,1,npixels,finalimg,&status)) printerror(status);

  fits_close_file(outptr,&status);

  return(0);

}
