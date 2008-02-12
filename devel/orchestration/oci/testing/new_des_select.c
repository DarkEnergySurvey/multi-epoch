#include "imageproc.h"
#include "desw_rel.h"



#define convert M_PI/180.0
#define DECAM 62
#define MOSAIC 8
#define BUFFERSIZE 6.0/60.0 // in 6 arcmin
#define TELESCOPE "Blanco 4m"


#define LINK "des"
#define USER "pipeline"
#define PASS "dc01user"

main(argc,argv)
int argc;
char *argv[];
{
  char imagename[1500],fullname[1500],line[1000],usnob_cat[500],nite[100],
  telescopename[500],detector[500],sqlcall[1000],temp[1000],imag_prev[1500];
  int ccdtotal=DECAM, *ccd, ccdnum, i, j, flag_quiet=0;
  float *raoff, *decoff, *rahw, *dechw;
  double ra, dec, ralow, rahigh, declow, dechigh;
  double ra_ori, dec_ori, ra_max, ra_min, dec_max, dec_min;

  char stmt[1000];
  char stmt2[1000];

  desw_ctx ctx;
  desw_ctx ctx2;

  r_set rs1;
  r_set rs2;
  sword retval = 0;

  int k;

  char tmp2[2000];

  FILE *fin, *fout, *pip;

  if(argc < 2)
  {
    printf("%s <image list> -detector <detector (DECam or Mosaic2)> -catalog <output usnob.cat> -nite <nite> -quiet\n",argv[0]);
    exit(0);
  }

  for(i=2;i<argc;i++)
  {
    if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
    if (!strcmp(argv[i],"-catalog")) sprintf(usnob_cat,"%s",argv[i+1]);
    if (!strcmp(argv[i],"-nite")) sprintf(nite,"%s",argv[i+1]);
    if (!strcmp(argv[i],"-detector"))
    {
      sprintf(detector,"%s",argv[i+1]);
      if (!strcmp(detector,"DECam")) ccdtotal=DECAM;
      else if(!strcmp(detector,"Mosaic2")) ccdtotal=MOSAIC;
      else
      {
        printf(" ** %s error: wrong input for -detector, abort\n",argv[0]);
        exit(0);
      }
    }
  }

  /* hardwired at the moment */
  sprintf(telescopename, "%s", TELESCOPE);

  /* memory allocatio for the wcsoffset info */
  ccd=(int *)calloc(ccdtotal+1,sizeof(int));
  raoff=(float *)calloc(ccdtotal+1,sizeof(float));
  decoff=(float *)calloc(ccdtotal+1,sizeof(float));
  rahw=(float *)calloc(ccdtotal+1,sizeof(float));
  dechw=(float *)calloc(ccdtotal+1,sizeof(float));

  initialize(&ctx, (text *)USER, (text *)PASS, (text *)LINK);
  initialize(&ctx2, (text *)USER, (text *)PASS, (text *)LINK);

  /* input wcsoffset info */

  sprintf( stmt, (text *)"SELECT chipid,raoffset,decoffset,rahwidth,dechwidth FROM wcsoffset WHERE TELESCOPE='%s' and DETECTOR='%s' ORDER BY chipid\n", telescopename,detector);

  sql_select_execute(&ctx, (text *)stmt, &rs1);

  j = 1;

  while( (retval = fetch( &ctx, &rs1 )) == OCI_SUCCESS ||
      (retval == OCI_SUCCESS_WITH_INFO) )
  {
    for (i = 0; i < rs1.parmcnt; i++)
    {
      if (strlen((void *)rs1.value[i]) != 0)
      {
        strcat( tmp2, rs1.value[i] );
        strcat( tmp2, " " );
      }
    }

    sscanf(tmp2,"%d %f %f %f %f",&ccd[j],&raoff[j],&decoff[j],&rahw[j],&dechw[j]);
    j++;
    tmp2[0] = '\0';
  }

  /* get the imagename and ccd from the list */

  fin=fopen(argv[1],"r");

  sprintf( stmt, (text *)"SELECT distinct ra,dec FROM files WHERE (upper(imagetype)='OBJECT' OR imagetype='reduced') AND NITE='%s' ", nite);

  j=0;
  while (fscanf(fin,"%s",fullname)!=EOF)
  {

    if (strncmp(&(fullname[strlen(fullname)-5]),".fits",5))
    {
      printf("  ** File must contain list of FITS images **\n");
      exit(0);
    }

    /* either ../imagename/imagename_ccd.fits or ../imagename/imagename_ccd_im.fits */
    if(!strncmp(&(fullname[strlen(fullname)-8]),"_im.fits",8))
    fullname[strlen(fullname)-8]=0;
    else
    fullname[strlen(fullname)-5]=0;

    for (i=strlen(fullname);i--;)
    {
      if (!strncmp(&(fullname[i]),"_",1))
      {
        fullname[i]=32;
        break;
      }
    }
    for (i=strlen(fullname);i--;)
    {
      if (!strncmp(&(fullname[i]),"/",1))
      {
        fullname[i]=32;
        break;
      }
    }
    sscanf(fullname,"%s %s %d",temp,imagename,&ccdnum);

    if(!j)
    {
      sprintf(stmt,"%s AND (imagename='%s'",stmt, imagename);
      sprintf(imag_prev,"%s",imagename);
    }
    else
    {
      if(strcmp(imag_prev,imagename))
      sprintf(stmt, "%s OR imagename='%s'", stmt, imagename);

      sprintf(imag_prev,"%s",imagename);
    }
    j++;
  }
  sprintf(stmt ,"%s)", stmt);
  fclose(fin);

  //fout = fopen( usnob_cat, "w" );
  fout = fopen( "temp_out.out", "w" );

  sql_select_execute(&ctx, (text *)stmt, &rs1);

  while( (retval = fetch( &ctx, &rs1 )) == OCI_SUCCESS ||
      (retval == OCI_SUCCESS_WITH_INFO) )
  {

    ra = atof( rs1.value[0] );
    dec = atof( rs1.value[1] );

    ra_ori = ra;
    dec_ori = dec;

    ra_min=360; ra_max=0.0;
    dec_min=90; dec_max=-90;
    for(i=1;i<=ccdtotal;i++)
    {

      ra = ra_ori + raoff[i]/cos(dec*convert);
      ralow = ra - rahw[i]/cos(dec*convert) - BUFFERSIZE;
      rahigh = ra + rahw[i]/cos(dec*convert) + BUFFERSIZE;

      dec = dec_ori + decoff[i];
      declow = dec - dechw[i] - BUFFERSIZE;
      dechigh = dec + dechw[i] + BUFFERSIZE;

      if(ralow < ra_min) ra_min=ralow;
      if(rahigh > ra_max) ra_max=rahigh;

      if(declow < dec_min) dec_min=declow;
      if(dechigh > dec_max) dec_max=dechigh;

    }

    sprintf( stmt2, "SELECT distinct ra,dec,sra,sde,r2 FROM USNOB_CAT1 WHERE (r2 BETWEEN 10.0 AND 20.5) AND (ra BETWEEN %2.6f AND %2.6f AND dec BETWEEN %2.6f AND %2.6f)", ra_min, ra_max, dec_min, dec_max);

    sql_select_execute(&ctx2, (text *)stmt2, &rs2);

    while( (retval = fetch( &ctx2, &rs2 )) == OCI_SUCCESS ||
        (retval == OCI_SUCCESS_WITH_INFO) )
    {

      for (k = 0; k < rs2.parmcnt; k++)
      {
        if (strlen((void *)rs2.value[k]) != 0)
        {
          fprintf( fout, "%-*s ", rs2.pcoll[k], (void *)rs2.value[k], strlen((void *)rs2.value[k]));
        }
      }
      fprintf( fout, "\n" );
    }

  }

  fclose( fout );
  cleanup_db(&ctx);
  cleanup_db(&ctx2);

  char cmd[5000];

  sprintf( cmd, "sort -u temp_out.out > %s", usnob_cat );
  system( cmd );
  system( "rm temp_out.out" );

  return(0);
}

#undef convert
#undef DECAM
#undef MOSAIC
#undef BUFFERSIZE
#undef TELESCOPE

