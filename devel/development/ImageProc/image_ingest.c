/* This program will read information from FITS file headers and 
/* create an output
/* file that is appropriate for ingestion into the FILES table */

/* has to work on one level up of runid */

#include "imageproc.h"

main(argc,argv)
	int argc;
	char *argv[];
{
	FILE	*inp,*out,*flpt;
	char	filename[800],record[120],comment[FLEN_COMMENT],
	        strrightascension[20],imageclass[10],
	        strdeclination[20],imagename[200],image[800],root[800],
		filter[FLEN_VALUE],obstype[FLEN_VALUE],biassec[200],
	        imagetype[FLEN_VALUE],date[100],ampname_a[FLEN_VALUE],
		ampname_b[FLEN_VALUE],band[80],
	        framelong[200],ctype1[FLEN_VALUE],
		ctype2[FLEN_VALUE],type[FLEN_VALUE],listname[500],
		arnode[100],arroot[1000],arsites[100],dblogin[500],
		outputname[500];

	char	zerokeyval[100][FLEN_VALUE],zerocomment[100][FLEN_VALUE],
		zerokeyword[100][10]={"OBSERVAT","TELESCOP","TELRADEC",
		"TELRA","TELDEC",
		"HA","ZD","TELFOCUS","DETECTOR","WEATDATE","WINDSPD",
		"WINDDIR","AMBTEMP",
		"HUMIDITY","PRESSURE","DIMMSEE","OBSERVER","PROPID",
		"OBJECT","",""},
		rakeyword[40],deckeyword[40],nite[200],
		tmp[200],airmasskeyword[200],runid[200],tilename[800];   
	int	status=0,anynull,nfound,keysexist,morekeys,i,rah,ram,decd,decm,
		imtype,hdunum,quiet,x,y,hdutype,chdu,j,len,crpix=1,ltv,keynum,
		flag_pv=0,flag_cd=0,flag_hms,
		*hdultv,flag,wcsdim=2,mef_flag,chip_i,count,ltv1,ccdnum=0;
	int	N_OBSERVAT=0,N_TELESCOP=1,N_TELRADEC=2,N_TELRA=3,N_TELDEC=4,
		N_HA=5,N_ZD=6,N_TELFOCUS=7,N_DETECTOR=8,N_WEATDATE=9,
		N_WINDSPD=10,
		N_WINDDIR=11,N_AMBTEMP=12,N_HUMIDITY=13,N_PRESSURE=14,
		N_DIMMSEE=15,
                N_OBSERVER=16,N_PROPID=17,N_OBJECT=18,flag_quiet=0,
		flag_remap=0,flag_reduced=0,flag_cat=0,flag_raw=0,flag_coadd=0,
		nslash,ndot,flag_ccd=0,select_ccdnum;
	float	exptime,airmass=1.0,darktime,equinox=2000.0,*hdurdnoise,
		*hdugain,arcsec,scampchi,skybrite,mnseeing,mdellipticity,
	        radecequinox=2000.0,ras,decs,rightascension,declination,
		ltm1=1.0,equinoxkeyword,gain,rdnoise,
	        gain_a,gain_b,rdnoise_a,rdnoise_b,saturate_a,saturate_b;
	double  crval1,crval2,crpix1,crpix2,cd1_1,cd2_1,cd1_2,cd2_2,ra,dec,
	        rho_a,rho_b,rho,cdelt1,cdelt2,pixscale,
	        raconvert(),decconvert();
	int	v;
	double	pv1[11],pv2[11];
	char	tag[20];
	long	axes[2],pixels,npixels,fpixel,photflag,scampnum,scampflg;
	void	printerror(),filename_resolve(),select_dblogin(),
		select_archivenode();

	fitsfile *fptr;

	if (argc<4) {
	  printf("image_ingest <fits-file list> <output> <archive node>\n");
	  printf("  -ccd <#>\n");
	  printf("  -quiet\n");
	  exit(0);
	}
	sprintf(listname,"%s",argv[1]);
	sprintf(outputname,"%s",argv[2]);
	sprintf(arnode,"%s",argv[3]);
	
	/* process the command line */
	for (i=4;i<argc;i++) {
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	  if (!strcmp(argv[i],"-ccd")) {
	    flag_ccd=1;
	    i++;
	    sscanf(argv[i],"%d",&select_ccdnum);
	  }
	}

	/* get dblogin info */
	select_dblogin(dblogin);
	/* get the archivesite string */
	select_archivenode(dblogin,arnode,arroot,arsites);

	out=fopen(outputname,"w");
	if (!flag_quiet) printf("  Opening output file %s\n",outputname);

	/* open input list */
	inp=fopen(listname,"r");
	if (inp==NULL) {
	  printf("  **FITS file list %s not found\n",listname);
	  exit(0);
	}
	/* cycle through the imagelist grabbing information and outputting it */
	while (fscanf(inp,"%s",filename)!=EOF) {

	  /* reset flags for each image */
	  flag_raw=flag_cat=flag_reduced=flag_remap=flag_coadd=0;
	  axes[0]=axes[1]=0;
	  exptime=darktime=crpix1=crpix2=0.0;

	  /* use imagename to extract the nite and actual name*/
	  /* assume form:  $runid/data/$nite/$type/imname/imname_##.fits */
	  /* so look between the 3rd and 4th "/" from the end */

          /* for remap image, the string is assumed to be */
          /* $runid/data/$nite/$type/imname/imname_##.tilename.fits */

	  
	  //filename_resolve(filename,runid,nite,band,tilename,imagetype,
	  //imagename,&ccdnum);
	
	  filename_resolve(filename,imageclass,runid,nite,tilename,imagetype,
	    imagename,band,&ccdnum);

	  if (!flag_ccd || (flag_ccd && ccdnum==select_ccdnum)) {
	    /* only process this file if it is of the correct ccd */
	    /* if ccd selection is turned on */

	  /* find out if it is a catalog fits file */
	  if (!strcmp( band,"raw")) {
	    flag_raw=1;
	    runid[0]=0;
	  }
	  else { 
	    if(!strcmp(imagetype,"catalog")) flag_cat=1; 
	    if(!strcmp(imagetype,"reduced")) flag_reduced=1; 
	    if(!strcmp(imagetype,"remap")) flag_remap=1; 
	    if(!strcmp(imagetype,"coadd")) flag_coadd=1; 
	  }

	  if (!flag_quiet) printf("  image_ingest:  runid: %s nite: %s band: %s tilename: %s imagetype: %s ccdnum: %d\n",runid,nite,band,tilename,imagetype,ccdnum);

	  /* open the FITS file */
	  if (fits_open_file(&fptr,filename,READONLY,&status)) 
	    printerror(status);
	  if (!flag_quiet) printf("  Opened %s",filename);	
	  
	  /* get the number of HDUs in the image */
	  if (fits_get_num_hdus(fptr,&hdunum,&status)) printerror(status);
	  if (!flag_quiet) printf(", which has  %d HDUs\n",hdunum);
	  
	  ra=dec=equinox=0.0; /* set up defaults */
	  /* get parameters we need for the FILES database table. */
	  if (fits_read_key_str(fptr,"DATE-OBS",date,comment,&status)==
	    KEY_NO_EXIST) {
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  DATE-OBS not found in %s\n",
		filename);
	    status=0;
	  }
	  else if (!flag_quiet) printf("  DATE-OBS:  %s",date);
	  
	  if (fits_read_key_str(fptr,"OBSTYPE",imagetype,comment,&status)==
	    KEY_NO_EXIST) {
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  OBSTYPE not found in %s\n",filename);
	    status=0;
	  }
	  else if (!flag_quiet) printf("  OBSTYPE=%s",imagetype);

	  if (fits_read_key_str(fptr,"FILTER",filter,comment,&status)==
	    KEY_NO_EXIST) {
	    if (!flag_quiet) 
	      printf("  \n  **image_ingest:  FILTER not found %s\n",filter);
	    sprintf(filter,"");
	    status=0;
	  }
	  /* just grab the first component of the filter name */
	  sscanf(filter,"%s",tmp);
	  sprintf(filter,"%s",tmp);
	  if (!flag_quiet) printf("  Filter=%s\n",filter);

	  if (fits_read_key_flt(fptr,"AIRMASS",&airmass,comment,&status)==
	    KEY_NO_EXIST) {
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  AIRMASS not found %.4f\n",airmass);
	    airmass=0.0;
	    status=0;
	  }
	  if (!flag_quiet) printf("  Z=%0.3f",airmass);
	    
	  if (fits_read_key_str(fptr,"TELRA",rakeyword,comment,&status)==
	    KEY_NO_EXIST){
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  TELRA not found  %s\n",rakeyword);
	    ra=0.0;
	    status=0;
	  }
	  else {
	    flag_hms=0;
	    for (i=0;i<strlen(rakeyword);i++) 
	      if (!strncmp(rakeyword+i,":",1)) {
		flag_hms=1;
	        break;
	      }
	    if (flag_hms) ra=raconvert(rakeyword,&rah,&ram,&ras);
	    else sscanf(rakeyword,"%lf",&ra);
	  }
	    
	  if (fits_read_key_str(fptr,"TELDEC",deckeyword,comment,&status)==
	    KEY_NO_EXIST){
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  TELDEC not found  %s\n",deckeyword);
	    dec=0.0;
	    status=0;
	  }
	  else {
	    flag_hms=0;
	    for (i=0;i<strlen(deckeyword);i++) 
	      if (!strncmp(deckeyword+i,":",1)) {
		flag_hms=1;
	        break;
	      }
	    if (flag_hms) dec=decconvert(deckeyword,&decd,&decm,&decs);
	    else sscanf(deckeyword,"%lf",&dec);
	  }
	  
	  radecequinox=equinox=0.0;
	  if (fits_read_key_flt(fptr,"TELEQUIN",&radecequinox,
	    comment,&status)==KEY_NO_EXIST){
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  TELEQUIN not found\n");
	    status=0;
	  }
	  if (fits_read_key_flt(fptr,"EQUINOX",&equinox,comment,
	    &status)==KEY_NO_EXIST){
	    if (!flag_quiet) printf("\n  **image_ingest:  EQUINOX not found\n");
	    status=0;
	  }
	  if (equinox<1.0e-5 && radecequinox<1.0e-5) 
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  No equinox information found\n");
	  else {
	    if (equinox<1.0e-5) equinox=radecequinox;
	    if (radecequinox<1.0e-5) radecequinox=equinox;
	  }
	  if (!flag_quiet) 
	    printf("  Pointing:  %s (%0.7f)  %s(%0.7f) %.0f\n",rakeyword,ra,deckeyword,dec,equinox);
	  
	  /* get exposure and darktime */
	  if (fits_read_key_flt(fptr,"EXPTIME",&exptime,comment,&status)==
	     KEY_NO_EXIST){
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  EXPTIME not found  %f\n",exptime);
	    exptime=0.0;
	    status=0;
	  }

	  if (fits_read_key_flt(fptr,"DARKTIME",&darktime,comment,&status)==
	    KEY_NO_EXIST){
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  DARKTIME not found  %f\n",darktime);
	    darktime=0.0;
	    status=0;
	  }
	  
	  /* read some other header parameters */
	  for (keynum=0;keynum<100;keynum++) if (!strlen(zerokeyword[keynum])) 
	    break;
	  if (!flag_quiet) 
	    printf("  Reading %d keywords from 0th HDU\n",keynum);
	  for (i=0;i<keynum;i++) {
	    if (fits_read_key_str(fptr,zerokeyword[i],zerokeyval[i],
	      zerocomment[i],&status)==KEY_NO_EXIST) {
	      if (!flag_quiet) printf("\n  **image_ingest:  %s not found\n",
		zerokeyword[i]);
	      sprintf(zerokeyval[i],"");
	      status=0;
	    }
	  }

	  /* telescope keyword alteration for BCS data */
	  if(!strcmp(zerokeyval[N_TELESCOP],"CTIO 4.0 meter telescope")) 
	    sprintf(zerokeyval[N_TELESCOP],"Blanco 4m");

	  /* remove error cases from DIMMSEE */
	  if (!strncmp(zerokeyval[N_DIMMSEE],"mysql_",6)) 
	    sprintf(zerokeyval[N_DIMMSEE],"");
	  
	  /* read the NAXIS1 and NAXIS2 keyword to get image size */
	  if (fits_read_keys_lng(fptr,"NAXIS",1,2,axes,&nfound,&status))
	    printerror(status);
	  if (!flag_quiet) printf("  %d X %d image\n",axes[0],axes[1]);
	  
	  
	  if (fits_read_key_flt(fptr,"SATURATE",&saturate_a,comment,&status)==
	    KEY_NO_EXIST) {
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  SATURATE not found\n");
	    saturate_b=65535.0;
	    status=0;
	  }
	  if (fits_read_key_str(fptr,"AMPNAMEA",ampname_a,comment,&status)==
	    KEY_NO_EXIST) {
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  AMPNAME_A not found\n");
	    sprintf(ampname_a,"");
	    status=0;
	  }
	  if (fits_read_key_str(fptr,"AMPNAMEB",ampname_b,comment,&status)==
	    KEY_NO_EXIST) {
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  AMPNAME_B not found\n");
	    sprintf(ampname_b,"");
	    status=0;
	  }
	  if (!flag_quiet) 
	    printf("  Amp %s and %s: Saturation (%.0f)\n",
	    ampname_a,ampname_b,saturate_a);
	  
	  /* read the WCS information */
	  if (fits_read_key_str(fptr,"CTYPE1",ctype1,comment,&status)==
	    KEY_NO_EXIST) {
	    if (!flag_quiet) printf("\n  **image_ingest:  CTYPE1 not found\n");
	    sprintf(ctype1,"");
	    status=0;
	  }
	  if (fits_read_key_str(fptr,"CTYPE2",ctype2,comment,&status)==
	    KEY_NO_EXIST) {
	    if (!flag_quiet) printf("\n  **image_ingest:  CTYPE2 not found\n");
	    sprintf(ctype2,"");
	    status=0;
	  }
	  if (fits_read_key_dbl(fptr,"CRVAL1",&crval1,comment,&status)==
	    KEY_NO_EXIST) {
	    crval1=0.0;
	    status=0;
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  crval1 not found %.4f\n",crval1);
	  }
	  if (crval1<0.0) crval1+=360.0;

	  if (fits_read_key_dbl(fptr,"CRVAL2",&crval2,comment,&status)==
	    KEY_NO_EXIST) {
	    crval2=0.0;
	    status=0;
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  crval2 not found %.4f\n",crval2);
	  }
	  if (fits_read_key_dbl(fptr,"CRPIX1",&crpix1,comment,&status)==
	    KEY_NO_EXIST) {
	    crpix1=0.0;
	    status=0;
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  crpix1 not found %.4f\n",crpix1);
	  }
	  if (fits_read_key_dbl(fptr,"CRPIX2",&crpix2,comment,&status)==
	    KEY_NO_EXIST) {
	    crpix2=0.0;
	    status=0;
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  crpix2 not found %.4f\n",crpix2);
	  }
	  if (fits_read_key_dbl(fptr,"CD1_1",&cd1_1,comment,&status)==
	    KEY_NO_EXIST) {
	    cd1_1=0.0;
	    status=0;
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  cd1_1 not found %.4f\n",cd1_1);
	  }
	  if (fits_read_key_dbl(fptr,"CD1_2",&cd1_2,comment,&status)==
	    KEY_NO_EXIST) {
	    cd1_2=0.0;
	    status=0;
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  cd1_2 not found %.4f\n",cd1_2);
	  }
	  if (fits_read_key_dbl(fptr,"CD2_1",&cd2_1,comment,&status)==
	    KEY_NO_EXIST) {
	    cd2_1=0.0;
	    status=0;
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  cd2_1 not found %.4f\n",cd2_1);
	  }
	  if (fits_read_key_dbl(fptr,"CD2_2",&cd2_2,comment,&status)==
	    KEY_NO_EXIST) {
	    cd2_2=0.0;
	    status=0;
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  cd2_2 not found %.4f\n",cd2_2);
	  }

	  for (v=0;v<=10;v++) {
	    sprintf(tag,"PV1_%d",v);
	    if (fits_read_key_dbl(fptr,tag,&(pv1[v]),comment,&status)==
	      KEY_NO_EXIST) {
	      pv1[v]=0.0;
	      status=0;
	      if (!flag_quiet) 
	        printf("\n  **image_ingest:  %s not found\n",tag);
	    }
	    if (pv1[v]>1.0) {
	      pv1[v]=0.0;
	      printf("  ** Warning:  PV1[%d]>1\n",v);
	    }
	    sprintf(tag,"PV2_%d",v);
	    if (fits_read_key_dbl(fptr,tag,&(pv2[v]),comment,&status)==
	      KEY_NO_EXIST) {
	      pv2[v]=0.0;
	      status=0;
	      if (!flag_quiet) 
	        printf("\n  **image_ingest:  %s not found\n",tag);
	    }
	    if (fabs(pv2[v])>=10. || fabs(pv1[v])>10.0) {
	      flag_pv=1;
	      printf("  ** Warning:  PV2[%d]>=10 %9.3e or PV1[%d]>=10.0 %9.3e \n",
		v,pv2[v],v,pv1[v]);
	    }
	  }
	  /* set all PVs to zero if one if out of range */
	  if (flag_pv) {
	    for (v=0;v<=10;v++) pv1[v]=pv2[v]=0.0;
	    if (!flag_quiet) printf(" **Warning: bad PV terms %s\n",
		filename);
	  }
	  /* test cd#_# and crval#, too */
	  arcsec=1.0/3600.0;
	  if (fabs(cd1_1)>arcsec || fabs(cd1_2)>arcsec ||
	    fabs(cd2_1)>arcsec || fabs(cd2_2)>arcsec || 
	    fabs(crval2)>90.0 || crval1<0.0 || crval1>360.0) {
	    flag_cd=1;
	    if (!flag_quiet) printf(" **Warning: bad WCS solution %s\n",
		filename);
	    /* bad solution-- set all information to zero */
	    cd1_1=cd1_2=cd2_1=cd2_2=crval1=crval2=crpix1=crpix2=0.0;
	    ctype1[0]=ctype2[0]=0;
	  }

	  /* read in image quality flags */
	  if (fits_read_key_flt(fptr,"SCAMPCHI",&scampchi,comment,&status)==
	     KEY_NO_EXIST){
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  SCAMPCHI not found  %f\n",scampchi);
	    scampchi=0.0;
	    status=0;
	  }
	  if (fits_read_key_lng(fptr,"SCAMPNUM",&scampnum,comment,&status)==
	     KEY_NO_EXIST){
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  SCAMPNUM not found  %f\n",scampnum);
	    scampnum=0;
	    status=0;
	  }
	  if (fits_read_key_lng(fptr,"SCAMPFLG",&scampflg,comment,&status)==
	     KEY_NO_EXIST){
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  SCAMPFLG not found  %f\n",scampflg);
	    scampflg=0;
	    status=0;
	  }
	  if (fits_read_key_lng(fptr,"PHOTFLAG",&photflag,comment,&status)==
	     KEY_NO_EXIST){
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  PHOTFLAG not found  %f\n",photflag);
	    photflag=0;
	    status=0;
	  }
	  if (fits_read_key_flt(fptr,"SKYBRITE",&skybrite,comment,&status)==
	     KEY_NO_EXIST){
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  SKYBRITE not found  %f\n",skybrite);
	    skybrite=0.0;
	    status=0;
	  }
	  if (fits_read_key_flt(fptr,"FWHM",&mnseeing,comment,&status)==
	     KEY_NO_EXIST){
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  FWHM not found  %f\n",mnseeing);
	    mnseeing=0.0;
	    status=0;
	  }
	  if (fits_read_key_flt(fptr,"ELLIPTIC",&mdellipticity,comment,
	    &status)== KEY_NO_EXIST){
	    if (!flag_quiet) 
	      printf("\n  **image_ingest:  ELLIPTIC not found\n");
	    mdellipticity=2.0;
	    status=0;
	  }


	  /* clear some fields if they are not needed */
	  if (flag_raw) runid[0]=0;
	  if (flag_remap) sprintf(imagetype,"remap");
	  if (flag_reduced) sprintf(imagetype,"reduced");
	  if (flag_cat) sprintf(imagetype,"catalog");
	  if (flag_coadd) sprintf(imagetype,"coadd");


	  /* Choong's addition */
	  if(flag_coadd) {
	    ra=crval1;
	    dec=crval2;	    
	  }

	  /* evaluate rho_a and rho_b as in Calabretta & Greisen (2002), eq 191 */
	  if(cd2_1>0) rho_a=atan(cd2_1/cd1_1);
	  else if(cd2_1<0) rho_a=atan(-cd2_1/-cd1_1);
	  else rho_a=0.0;
	  
	  if(cd1_2>0) rho_b=atan(cd1_2/-cd2_2);
	  else if(cd1_2<0) rho_b=atan(-cd1_2/cd2_2);
	  else rho_b=0.0;
	  
	  /* evaluate rho and CDELTi as in Calabretta & Greisen (2002), eq 193 */
	  rho=0.5*(rho_a+rho_b);
	  cdelt1=cd1_1/cos(rho);
	  cdelt2=cd2_2/cos(rho);
	  
	  /* convert the pixel to arcsec */
	  pixscale=0.5*(fabs(cdelt1)+fabs(cdelt2))*3600;
	  if(pixscale==0.0)
	    printf(" ** image_ingest WARNING: pixscale = 0\n");
	  mnseeing*=pixscale;
	  
	  /* end Choong's addition */

	  /* make sure filter is correct */
	  if (!strlen(filter) && strcmp(band,"raw")) 
	    sprintf(filter,"%s",band);
	  
	  /* output database ingest line */
	  for (i=0;i<2;i++) {
	    if (!i) flpt=out; /* output to file */
	    else if (i && !flag_quiet) flpt=stdout; /* output to stdout */
	    else break;
	    fprintf(flpt,"%.6f|%.6f|%.2f|%s|%.1f|%.1f|%.1f|%.1f|%0.3f|",
	      ra,dec,radecequinox,date,gain_a,rdnoise_a,gain_b,
	      rdnoise_b,airmass);
	    fprintf(flpt,"%s|%s|%s|%s|%s|%s|%d|%.3f|%.3f|",
	      filter,imagetype,imagename,runid,tilename,nite,
	      ccdnum,exptime,darktime);
	    fprintf(flpt,"%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|",
	      zerokeyval[N_OBJECT],
	      zerokeyval[N_OBSERVAT],zerokeyval[N_TELESCOP],
	      zerokeyval[N_HA],
	      zerokeyval[N_ZD],zerokeyval[N_DETECTOR],
	      zerokeyval[N_OBSERVER],
	      zerokeyval[N_PROPID],zerokeyval[N_WEATDATE],
	      zerokeyval[N_WINDSPD],
	      zerokeyval[N_WINDDIR],zerokeyval[N_AMBTEMP],
	      zerokeyval[N_HUMIDITY],
	      zerokeyval[N_PRESSURE],zerokeyval[N_DIMMSEE]);
	    fprintf(flpt,"%7.2f|%d|%s|%s|%10.7f|%10.7f|",
		   equinox,wcsdim,ctype1,ctype2,crval1,crval2);
	    fprintf(flpt,"%10.4f|%10.4f|%19.13e|%19.13e|%19.13e|%19.13e|",
		   crpix1,crpix2,cd1_1,cd2_1,cd1_2,cd2_2);
	    fprintf(flpt,"%11.10e|%11.10e|%11.10e|%11.10e|%11.10e|%11.10e|",
		   pv1[0],pv1[1],pv1[2],pv1[3],pv1[4],pv1[5]);
	    fprintf(flpt,"%11.10e|%11.10e|%11.10e|%11.10e|%11.10e|",
		   pv1[6],pv1[7],pv1[8],pv1[9],pv1[10]);
	    fprintf(flpt,"%11.10e|%11.10e|%11.10e|%11.10e|%11.10e|%11.10e|",
		   pv2[0],pv2[1],pv2[2],pv2[3],pv2[4],pv2[5]);
	    fprintf(flpt,"%11.10e|%11.10e|%11.10e|%11.10e|%11.10e|",
		   pv2[6],pv2[7],pv2[8],pv2[9],pv2[10]);
	    fprintf(flpt,"%d|%d|%d|",axes[0],axes[1],hdunum);
	    fprintf(flpt,"%d|%.2f|%d|%d|%.3f|%.3f|%.4f|%s\n",
	      scampnum,scampchi,scampflg,photflag,skybrite,
	      mnseeing,mdellipticity,arsites);
	    fflush(flpt);
	  }

	  
          /* close the new image */
	  if (fits_close_file(fptr,&status)) printerror(status);
	  if (!flag_quiet) printf("  Closed image %s\n",filename);
	  
	  } /* end of ccd selection conditional */
	} /* while loop */
	  
	fclose(inp);
	fclose(out);
	return(0);
}
