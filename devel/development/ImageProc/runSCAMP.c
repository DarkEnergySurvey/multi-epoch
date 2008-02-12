/* 

steps:
1. read list and differentiate imagename
2. mkdir for each imagename
3. call fitscombine for each imagename
4. call scamp for each imagename
5. read .head
6. insert .head info back to reduced image

Note: assume the input list looks like:   imagename band ccdnumber path
      decam--11--38-i-04 i 03  DES20061127_/data/des20061010/i/decam--11--38-i-04/decam--11--38-i-04_03
      and order with imagename
      assume there's database access

*/

#include "imageproc.h"

#define IMMAX 500 /* maximum of imagename(exposure) per nite */
#define DECAM 62
#define MOSAIC 8

main(argc,argv)
        int argc;
        char *argv[];
{
  char archpath[1000],dirlocation[1000],command[50000],line[1000],temp[1000],dirbase[1000];
  char outfitslist[1000],outcatslist[1000],outbpmlist[1000],outvarlist[1000],band_prev[10];
  char imagename[500],imagename_prev[500],telescope[200],detector[200],usnob_cat[1200];
  char binpath[1000],etcpath[1000],server[100],nite[300],incat[1200],wlog[1200];
  char aheadfile[1000],aheadname[1000],image_ahead[1000],comment[1000];
  char fullfits[1000],fullbpm[1000],fullvar[1000],headname[1000];
  char band[5],impath[1500],**impatharray,**imarray,**bandarray;
  char rastring[100],decstring[100],mosaictype[100],command2[5000],commandout[5000];
  int i,j,k,idx,len,flag,count=0,imnum=0,imcount=0,nimage,nstar,flag_repeat;
  int flag_quiet=0,flag_server=0,flag_incat=0,flag_localcat=0,flag_matchresol=0;
  int flag_distort_deg=0,flag_crossrad=0,flag_fgroup=0,flag_nsigma=0;
  int flag_positionerr=0,flag_posangleerr=0,flag_pixscaleerr=0,flag_usnobcat=0;
  int flag_ahead=0,flag_outahead=0,flag_noradec=0,flag_crval=0,flag_hms=0;
  int flag_warningmssg=0,status=0,check_mosaictype=0;
  int *ccdarray,ccd=0,ccdtotal=DECAM,distort_deg;
  int rah,ram,dech,decm;
  float ras,decs;
  float crossrad,fgroup_radius,astrclip_nsigma,match_resol;
  float pixscale_maxerr,posangle_maxerr,position_maxerr;
  float match_resol_input,position_maxerr_input,chi2;
  float *match_resolgrid,*position_maxerrgrid;
  double ra,dec,raconvert(),decconvert();
  void printerror();
  time_t curtime=time (NULL);
  FILE *flist,*ffits,*fcats,*fbpm,*fvar,*fahead,*foutahead,*fhead,*fouthead,*pip,*fwarning,*fout,*fwlog;
  fitsfile *fptr;

  if (argc<2) {
    printf("Usage: %s <input_list> <output_dir> <archive_path> \n",argv[0]);
    printf("            -binpath <binpath>\n");
    printf("            -etcpath <etcpath>\n");
    printf("            -detector <detector> (DECam or Mosaic2)\n");
    printf("          Options:\n");
    printf("            -ahead <template ahead>\n");
    printf("            -quiet\n");
    printf("          Scamp options:\n");
    printf("            -mosaictype <UNCHANGED,SAME_CRVAL,SHARE_PROJAXIS,FIX_FOCALPLANE,LOOSE; default:SAME_CRVAL>\n");
    printf("            -distort_degree <#>\n");
    printf("            -crossid_radius <#>\n");
    printf("            -astrclip_nsigma <#>\n");
    printf("            -fgroup_radius <#>\n");
    printf("            -match_resol <#>\n");
    printf("            -pixscale_maxerr <#>\n");
    printf("            -posangle_maxerr <#>\n");
    printf("            -position_maxerr <#>\n");
    printf("            -server <server_name>\n");
    printf("            -catalog <NONE,USNO-A1,USNO-A2,USNO-B1,GSC-1.3,GSC-2.2,UCAC-1,UCAC-2,2MASS (default:USNO-B1)>\n");
    printf("            -inputcat <fits catalog>\n");
    printf("            -localcat (querying USNO-B1 catalog from DB)\n");
    printf("          Compulsory options if -localcat is set:\n");
    printf("            -nite <nite> \n");
    exit(0);
  }

  sprintf(dirbase,"%s",argv[2]);
  sprintf(archpath,"%s",argv[3]);

  sprintf(mosaictype,"%s","SAME_CRVAL");

  for(i=4;i<argc;i++) {
    if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
    if (!strcmp(argv[i],"-detector")) {
      sprintf(detector,"%s",argv[i+1]);
      if (!strcmp(detector,"DECam")) ccdtotal=DECAM;
      else if(!strcmp(detector,"Mosaic2")) ccdtotal=MOSAIC;
      else {
	printf(" ** %s error:  -detector must be either DECam or Mosaic2\n",argv[0]);
	exit(0);
      }
    }
    if (!strcmp(argv[i],"-binpath")) 
      sprintf(binpath,"%s",argv[i+1]);
    if (!strcmp(argv[i],"-etcpath")) 
      sprintf(etcpath,"%s",argv[i+1]);

    if (!strcmp(argv[i],"-ahead")) {
      flag_ahead=1;
      sprintf(aheadfile,"%s",argv[i+1]);
    }

    /* scamp options */
    if (!strcmp(argv[i],"-mosaictype")) {
      sprintf(mosaictype,"%s",argv[i+1]);
    }    

    if (!strcmp(argv[i],"-crossid_radius")) {
      flag_crossrad=1;
      sscanf(argv[i+1],"%f",&crossrad);
    }
    if (!strcmp(argv[i],"-fgroup_radius")) {
      flag_fgroup=1;
      sscanf(argv[i+1],"%f",&fgroup_radius);
    }
    if (!strcmp(argv[i],"-astrclip_nsigma")) {
      flag_nsigma=1;
      sscanf(argv[i+1],"%f",&astrclip_nsigma);
    }
    if (!strcmp(argv[i],"-match_resol")) {
      flag_matchresol=1;
      sscanf(argv[i+1],"%f",&match_resol);
      match_resol_input=match_resol;
    }
    if (!strcmp(argv[i],"-pixscale_maxerr")) {
      flag_pixscaleerr=1;
      sscanf(argv[i+1],"%f",&pixscale_maxerr);
    }
    if (!strcmp(argv[i],"-posangle_maxerr")) {
      flag_posangleerr=1;
      sscanf(argv[i+1],"%f",&posangle_maxerr);
    }
    if (!strcmp(argv[i],"-position_maxerr")) {
      flag_positionerr=1;
      sscanf(argv[i+1],"%f",&position_maxerr);
      position_maxerr_input=position_maxerr;
    }
    if (!strcmp(argv[i],"-distort_degree")) {
      flag_distort_deg=1;
      distort_deg=atoi(argv[i+1]);
    }
    if (!strcmp(argv[i],"-server")) {
      flag_server=1;
      sprintf(server,"%s",argv[i+1]);
    }
    if (!strcmp(argv[i],"-catalog")) {
      flag_incat=1;
      sprintf(incat,"%s",argv[i+1]);
    }
    if (!strcmp(argv[i],"-localcat")) {
      flag_incat=0;
      flag_usnobcat=0;
      flag_localcat=1;
    }
    if (!strcmp(argv[i],"-inputcat")) {
      flag_incat=0;
      flag_localcat=0;
      flag_usnobcat=1;
      sprintf(usnob_cat,"%s",argv[i+1]);
    }

    if (!strcmp(argv[i],"-telescope")) 
      sprintf(telescope,"%s",argv[i+1]);
    if (!strcmp(argv[i],"-nite")) 
      sprintf(nite,"%s",argv[i+1]);
  }

  /* print out the time of processing */
  if(!flag_quiet)
    printf(" ** Running %s on %s \n",argv[0],asctime(localtime (&curtime)));

  /* memory allocation */
  ccdarray=(int *)calloc(ccdtotal+1,sizeof(int));
  impatharray=(char **)calloc(ccdtotal+1,sizeof(char *));
  for(i=ccdtotal+1;i--;)
    impatharray[i]=(char *)calloc(1500,sizeof(char));
  imarray=(char **)calloc(IMMAX,sizeof(char *));
  for(i=IMMAX;i--;)
    imarray[i]=(char *)calloc(1000,sizeof(char));
  bandarray=(char **)calloc(IMMAX,sizeof(char *));
  for(i=IMMAX;i--;)
    bandarray[i]=(char *)calloc(10,sizeof(char));

  match_resolgrid=(float *)calloc(4,sizeof(float));
  position_maxerrgrid=(float *)calloc(6,sizeof(float));

  /* initialize the grids */
  match_resolgrid[0]=0;  match_resolgrid[1]=1;  
  match_resolgrid[2]=2;  match_resolgrid[3]=5; 

  position_maxerrgrid[0]=1.0;  position_maxerrgrid[1]=2.0;
  position_maxerrgrid[2]=5.0;  position_maxerrgrid[3]=10.0;
  position_maxerrgrid[4]=20.0; position_maxerrgrid[5]=60.0;

  /* count number of images in the list */
  flist=fopen(argv[1],"r");
  while (fscanf(flist,"%s %s %d %s",imagename, band, &ccd, impath)!=EOF)
    imnum++;
  fclose(flist);  

  if(!flag_quiet) {
    if(!imnum) {
      printf(" ** %s error: no image found in input list %s, abort\n",argv[0],argv[1]);
      exit(0);
    }
    else
      printf(" ** Image list %s contains %d images\n",argv[1],imnum);
  }

  /* *** begin process *** */

  flist=fopen(argv[1],"r");

  if(imnum==1) {
    fscanf(flist,"%s %s %d %s",imagename, band, &ccd, impath);

    if(!flag_quiet) printf("%s\tN=%d\n",imagename,imnum);

    if(ccdtotal==DECAM) { 
      sprintf(dirlocation,"%s/%s",dirbase,band);

      if(!flag_quiet) printf("sub-dir %s/%s created\t",dirlocation,imagename);
      sprintf(command,"/bin/mkdir -p %s/%s",dirlocation,imagename);
      system(command);
    }
    sprintf(dirlocation,"%s/%s",dirbase,band);
	  
    sprintf(outfitslist,"%s/%s/%s_fits.list",dirlocation,imagename,imagename);
    sprintf(outcatslist,"%s/%s/%s_cats.list",dirlocation,imagename,imagename);
    sprintf(outbpmlist,"%s/%s/%s_bpm.list",dirlocation,imagename,imagename);
    sprintf(outvarlist,"%s/%s/%s_var.list",dirlocation,imagename,imagename);
      
    ffits=fopen(outfitslist,"w");
    fcats=fopen(outcatslist,"w");
    fbpm=fopen(outbpmlist,"w");
    fvar=fopen(outvarlist,"w");
    
    fprintf(ffits,"%s/%s_im.fits\n",archpath,impath);
    fprintf(fcats,"%s/%s_scamp.fits\n",archpath,impath);
    fprintf(fbpm,"%s/%s_bpm.fits\n",archpath,impath);
    fprintf(fvar,"%s/%s_var.fits\n",archpath,impath);
	
    fclose(ffits); fclose(fcats); fclose(fbpm); fclose(fvar);

    sprintf(imarray[0],"%s",imagename);
    sprintf(bandarray[0],"%s",band);

    imcount=imnum;
  }
  else {
    k=0;
    while(fscanf(flist,"%s %s %d %s",imagename, band, &ccd, impath)!=EOF) {
      k++;
      
      if(k==1) { /* for the first row in the list */ 
	
	if(!flag_quiet) printf("%s\t",imagename);
	
	/* create the sub-dir for the new imagename (exposure) for DECAM case */
	if(ccdtotal==DECAM) { 
	  sprintf(dirlocation,"%s/%s",dirbase,band);

	  if(!flag_quiet) printf("sub-dir %s/%s created\t",dirlocation,imagename);
	  sprintf(command,"/bin/mkdir -p %s/%s",dirlocation,imagename);
	  system(command);
	}
	
	sprintf(imarray[imcount],"%s",imagename);
	sprintf(bandarray[imcount],"%s",band);
	imcount++;
	
	/* initialize array */
	for(i=ccdtotal+1;i--;) ccdarray[i]=0;
	
	/* update info */
	count=1;
	ccdarray[ccd]=1;
	sprintf(impatharray[ccd],"%s",impath);
	sprintf(imagename_prev,"%s",imagename);	
	sprintf(band_prev,"%s",band);
      }
      else if(k==imnum) { /* for the last row in the list */
	count++;
	ccdarray[ccd]=1;
	sprintf(impatharray[ccd],"%s",impath);
	  
	if(!flag_quiet) printf("N=%d\n",count);
	
	/* output filelists for the last imagename */
	sprintf(dirlocation,"%s/%s",dirbase,band);
	
	sprintf(outfitslist,"%s/%s/%s_fits.list",dirlocation,imagename_prev,imagename_prev);
	sprintf(outcatslist,"%s/%s/%s_cats.list",dirlocation,imagename_prev,imagename_prev);
	sprintf(outbpmlist,"%s/%s/%s_bpm.list",dirlocation,imagename_prev,imagename_prev);
	sprintf(outvarlist,"%s/%s/%s_var.list",dirlocation,imagename_prev,imagename_prev);
      
	ffits=fopen(outfitslist,"w");
	fcats=fopen(outcatslist,"w");
	fbpm=fopen(outbpmlist,"w");
	fvar=fopen(outvarlist,"w");
      
	for(i=1;i<=ccdtotal;i++) {
	  if(ccdarray[i]) {
	    fprintf(ffits,"%s/%s_im.fits\n",archpath,impatharray[i]);
	    fprintf(fcats,"%s/%s_scamp.fits\n",archpath,impatharray[i]);
	    fprintf(fbpm,"%s/%s_bpm.fits\n",archpath,impatharray[i]);
	    fprintf(fvar,"%s/%s_var.fits\n",archpath,impatharray[i]);
	  }
	}
	fclose(ffits); fclose(fcats); fclose(fbpm); fclose(fvar);
      }
      else { /* for other rows in the list */
	
	/* compare current imagename with imagename_prev */
	if (strcmp(imagename,imagename_prev)) { /* if the imagenames are different */
	  
	  /* print out the image count for previous imagename */
	  if(!flag_quiet) printf("N=%d\n",count);

	  sprintf(dirlocation,"%s/%s",dirbase,band_prev);

	  /* output filelists */
	  sprintf(outfitslist,"%s/%s/%s_fits.list",dirlocation,
	    imagename_prev,imagename_prev);
	  sprintf(outcatslist,"%s/%s/%s_cats.list",dirlocation,
	    imagename_prev,imagename_prev);
	  sprintf(outbpmlist,"%s/%s/%s_bpm.list",dirlocation,
	    imagename_prev,imagename_prev);
	  sprintf(outvarlist,"%s/%s/%s_var.list",dirlocation,
	    imagename_prev,imagename_prev);
	  
	  ffits=fopen(outfitslist,"w");
	  fcats=fopen(outcatslist,"w");
	  fbpm=fopen(outbpmlist,"w");
	  fvar=fopen(outvarlist,"w");

	  for(i=1;i<=ccdtotal;i++) {
	    if(ccdarray[i]) {
	      fprintf(ffits,"%s/%s_im.fits\n",archpath,impatharray[i]);
	      fprintf(fcats,"%s/%s_scamp.fits\n",archpath,impatharray[i]);
	      fprintf(fbpm,"%s/%s_bpm.fits\n",archpath,impatharray[i]);
	      fprintf(fvar,"%s/%s_var.fits\n",archpath,impatharray[i]);
	    }
	  }
	  fclose(ffits); fclose(fcats); fclose(fbpm); fclose(fvar);
	  
	  if(!flag_quiet) printf("%s\t",imagename);
	  /* create the sub-dir for the new imagename for case DECAM */
	  if(ccdtotal==DECAM) {
	    sprintf(dirlocation,"%s/%s",dirbase,band);
	    
	    if(!flag_quiet) printf("sub-dir %s/%s created\t",dirlocation,imagename);
	    sprintf(command,"/bin/mkdir -p %s/%s",dirlocation,imagename);
	    system(command);
	  }

	  sprintf(imarray[imcount],"%s",imagename);
	  sprintf(bandarray[imcount],"%s",band);
	  imcount++;

	  /* initialize array */
	  for(i=ccdtotal+1;i--;) ccdarray[i]=0;
	  
	  /* update info */
	  count=1;
	  ccdarray[ccd]=1;
	  sprintf(impatharray[ccd],"%s",impath);
	  sprintf(imagename_prev,"%s",imagename);
	  sprintf(band_prev,"%s",band);
	}
	else { /* if the imagenames are the same */
	  count++;
	  ccdarray[ccd]=1;
	  sprintf(impatharray[ccd],"%s",impath);
	}
      }
    }
  }
  fclose(flist);

  if(!flag_quiet)
    printf(" ** Found %d imagename (exposure) in list: %s\n",imcount,argv[1]);

  /* scamp */
  sprintf(wlog,"%s/log/astrometryQA.log",dirbase);
  fwlog=fopen(wlog,"w");
  
  for(i=0;i<imcount;i++) {

    sprintf(dirlocation,"%s/%s",dirbase,bandarray[i]);
    
    /* !! run the runSExtractor with input list to produce the scamp catalog here ?? */

    /* run fitscombine to combine the *_scamp.fits catalog */
    sprintf(command,"%s/fitscombine -list %s/%s/%s_cats.list %s/%s/%s_scamp.fits -no0exthead ",
	    binpath,dirlocation,imarray[i],imarray[i],dirlocation,imarray[i],imarray[i]);
    if(flag_quiet) sprintf(command,"%s -quiet ",command);
    if(!flag_quiet) printf("%s\n",command);
    fflush(stdout);
    system(command);

    
    /* query DB for local USNO-B catalog */
    if(flag_localcat) {

      /* calling select_astrometric_standards and ascii2ldac */
      sprintf(command,"%s/select_astrometric_standards %s/%s/%s_fits.list -catalog %s/%s/%s_USNOB.cat -detector %s -nite %s ", 
	      binpath,dirlocation,imarray[i],imarray[i],dirlocation,imarray[i],imarray[i],detector,nite);
      if(flag_quiet) sprintf(command,"%s -quiet ",command);
      if(!flag_quiet) printf("%s\n",command);
      fflush(stdout);
      system(command);

      sprintf(command,"%s/ascii2ldac %s/%s/%s_USNOB.cat %s/%s/%s_USNOB.fits ", 
	      binpath,dirlocation,imarray[i],imarray[i],dirlocation,imarray[i],imarray[i]);
      if(flag_quiet) sprintf(command,"%s -quiet ",command);
      if(!flag_quiet) printf("%s\n",command);
      fflush(stdout);
      system(command);
    }

    /* create the .ahead file for this imagename from the template (input) .ahead file */
    if(flag_ahead) {
 
      for(idx=ccdtotal+1;idx--;) ccdarray[idx]=0;

      /* open the first image to read in (raw) ra,dec, assume is same for all ccds */
      /* or move to the while loop for each of the ccd ?? */
      sprintf(outfitslist,"%s/%s/%s_fits.list",dirlocation,imarray[i],imarray[i]);
      ffits=fopen(outfitslist,"r");
      fscanf(ffits,"%s",image_ahead);
      fclose(ffits);
      
      status=0;
      if(fits_open_file(&fptr,image_ahead,READONLY,&status)) printerror(status);

      flag_noradec=0; status=0;
      if(fits_read_key_str(fptr,"TELRA",rastring,comment,&status)==KEY_NO_EXIST) {
	flag_noradec=1;
	printf("** Image %s has no TELRA\n",image_ahead);
	printerror(status);
      }
      else {
	flag_hms=0;
	for (j=0;j<strlen(rastring);j++) {
	  //if (!strncmp(rastring+j,"'",1))
	  //rastring[j]=32;
	  if (!strncmp(rastring+j,":",1)) {
	    flag_hms=1;
	    break;
	  }
	}
	if (flag_hms) ra=raconvert(rastring,&rah,&ram,&ras); 
	else sscanf(rastring,"%lf",&ra);
      }
      if(fits_read_key_str(fptr,"TELDEC",decstring,comment,&status)==KEY_NO_EXIST) {
	printf("** Image %s has no TELDEC\n",image_ahead);
	flag_noradec=1;
	printerror(status);
      }
      else {
	flag_hms=0;
	for (j=0;j<strlen(decstring);j++)
	  if (!strncmp(decstring+j,":",1)) {
	    flag_hms=1;
	    break;
	  }
	if (flag_hms) dec=decconvert(decstring,&dech,&decm,&decs);
	else sscanf(decstring,"%lf",&dec);
      }
      if(fits_close_file(fptr,&status)) printerror(status);
      
      /* reopen for process */
      sprintf(outfitslist,"%s/%s/%s_fits.list",dirlocation,imarray[i],imarray[i]);
      ffits=fopen(outfitslist,"r");
      
      while(fscanf(ffits,"%s",image_ahead)!=EOF) {
	/* chopoff the _im.fits in image_ahead and find the ccd number */
	for(j=strlen(image_ahead);j--;) {
	  if(!strncmp(&(image_ahead[j]),"_im.fits",8)) 
	    image_ahead[j]=0;
	  if (!strncmp(&(image_ahead[j]),"_",1)) {
	    image_ahead[j]=32;
	    break;
	  }
	}
	
	sscanf(image_ahead,"%s %d",temp,&ccd);
	ccdarray[ccd]=1;
      }
      fclose(ffits);

      fahead=fopen(aheadfile,"r");
      sprintf(aheadname, "%s/%s/%s_scamp.ahead",dirlocation,imarray[i],imarray[i]);
      foutahead=fopen(aheadname,"w");

      /* get the WCS parameters from input .ahead file with particular ccd */
      while(fgets(line,1000,fahead)!=NULL) {
	sscanf(line,"%s %d",temp,&ccd);
	
	if (!strcmp(temp,"COMMENT")) { 
	  k=0;
	  if(ccdarray[ccd]) flag_outahead=1;
	  else flag_outahead=0;
	}
       
	/* not printing out the CRVAL in template .ahead */
	flag_crval=1;
	if (!strcmp(temp,"CRVAL1") || !strcmp(temp,"CRVAL2"))
	  flag_crval=0;

	if(flag_outahead) { 
	  if(flag_crval) {
	    fprintf(foutahead,"%s",line);
	    if(k<1 && !flag_noradec) {
	      fprintf(foutahead,"CRVAL1  =           %3.6f\n",ra);
	      fprintf(foutahead,"CRVAL2  =           %3.6f\n",dec);
	    }
	    k++;
	  }
	}
      }
      
      fclose(fahead); fclose(foutahead);
    }

    /* run scamp */
    sprintf(command,"%s/scamp %s/%s/%s_scamp.fits -c %s/default.scamp -MOSAIC_TYPE %s -WRITE_XML N ",
	    binpath,dirlocation,imarray[i],imarray[i],etcpath,mosaictype);
    sprintf(command2,"%s ",command);

    if(flag_quiet) { 
      sprintf(command,"%s -VERBOSE_TYPE QUIET ",command);
      sprintf(command2,"%s ",command);
    }
    if(flag_distort_deg) {
      sprintf(command,"%s -DISTORT_DEGREES %d ",command,distort_deg);
      sprintf(command2,"%s ",command);
    }
    if(flag_crossrad) {
      sprintf(command,"%s -CROSSID_RADIUS %2.2f ",command,crossrad);
      sprintf(command2,"%s ",command);
    }
    if(flag_fgroup) {
      sprintf(command,"%s -FGROUP_RADIUS %2.2f ",command,fgroup_radius);
      sprintf(command2,"%s ",command);
    }
    if(flag_nsigma) {
      sprintf(command,"%s -ASTRCLIP_NSIGMA %2.2f ",command,astrclip_nsigma);
      sprintf(command2,"%s ",command);
    }

    if(flag_pixscaleerr) {
      sprintf(command,"%s -PIXSCALE_MAXERR %2.2f ",command,pixscale_maxerr);
      sprintf(command2,"%s ",command);
    }
    if(flag_posangleerr) {
      sprintf(command,"%s -POSANGLE_MAXERR %2.2f ",command,posangle_maxerr);
      sprintf(command2,"%s ",command);
    }

    if(flag_server) {
      sprintf(command,"%s -REF_SERVER %s ",command,server);
      sprintf(command2,"%s ",command);
    }
    if(flag_incat) {
      sprintf(command,"%s -ASTREF_CATALOG %s ",command,incat);
      sprintf(command2,"%s ",command);
    }
    if(flag_localcat) {
      sprintf(command,"%s -ASTREF_CATALOG FILE ",command);
      sprintf(command,"%s -ASTREFCAT_NAME %s/%s/%s_USNOB.fits ",
	      command,dirlocation,imarray[i],imarray[i]); 
      sprintf(command2,"%s ",command);
    }
    else if(flag_usnobcat) {
      sprintf(command,"%s -ASTREF_CATALOG FILE ",command);
      sprintf(command,"%s -ASTREFCAT_NAME %s ",command,usnob_cat); 
      sprintf(command2,"%s ",command);
    }
    else {
      sprintf(command,"%s -CDSCLIENT_EXEC %s/aclient ",command,binpath);
      sprintf(command2,"%s ",command);
    }

    if(flag_matchresol)
      sprintf(command,"%s -MATCH_RESOL %2.2f ",command,match_resol);

    if(flag_positionerr) 
      sprintf(command,"%s -POSITION_MAXERR %2.2f ",command,position_maxerr);   

    sprintf(commandout,"%s",command);
    //sprintf(command,"%s > out.txt",command);
    sprintf(command,"%s ",command);

    fflush(stdout);

    /* check if there's any WARNING in the stderr */
    if (freopen("warning.txt","w",stderr) != NULL)
      system(command); 
    
    flag_repeat=0; flag_warningmssg=0;
    fwarning=fopen("warning.txt", "r");
    while(fgets(line,1000,fwarning) != NULL) {
      if(!strncmp(line,"> WARNING: Not enough matched detections in set",47)) {
	flag_repeat=1;
	flag_warningmssg=1;
      }
      if(!strncmp(line,"> WARNING: Significant inaccuracy likely to occur in projection",63)) {
	flag_repeat=1;
	flag_warningmssg=2;
      }
    }
    fclose(fwarning);

    //system("/bin/rm warning.txt");

    /* loop over the parameters (match_resol and position_maxerr) */
    if(flag_repeat) {
      for(j=0;j<4;j++) {
	for(k=0;k<6;k++) {
	
	  match_resol = match_resolgrid[j];
	  position_maxerr = position_maxerrgrid[k];

	  if(match_resol != match_resol_input || position_maxerr != position_maxerr_input) {
	    sprintf(command,"%s -MATCH_RESOL %2.2f -POSITION_MAXERR %2.2f ",command2,match_resol,position_maxerr);
	    sprintf(commandout,"%s",command);
	    sprintf(command,"%s > out.txt",command);
	    
	    //printf("%s\n",command);
	    if (freopen("warning.txt","w",stderr) != NULL)
	      system(command);
	
	    flag_repeat=0;
	    fwarning=fopen("warning.txt", "r");
	    while(fgets(line,1000,fwarning) != NULL) {
	      if (!flag_quiet) printf("%s",line);
	      if(!strncmp(line,"> WARNING: Not enough matched detections in set",47)) {
		flag_repeat=1; 
		flag_warningmssg=1;
	      }
	      if(!strncmp(line,"> WARNING: Significant inaccuracy likely to occur in projection",63)) {
		flag_repeat=1; 
		flag_warningmssg=2;
	      }
	      //if(flag_repeat)
	      //printf("WARNING message exists in loop for match_resol=%2.1f position_maxerr=%2.1f\n",flag_repeat,match_resol,position_maxerr);
	    }
	    fclose(fwarning);
    
	    //system("/bin/rm warning.txt");
	  }
	  if(!flag_repeat) break;
	} // k loop
	if(!flag_repeat) break;
      } // j loop
    }

    if(!flag_quiet) printf("%s\n",commandout);
    
    if(flag_repeat)
      printf(" -- WARNING message still exist for %s\n",imarray[i]);
 
    /* grep the Nstar (number of matched stars) and chi2 info */
    //pip=popen("/bin/grep --after-context=4 --regexp='Astrometric stats (external)' out.txt","r");
    pip=popen("/bin/grep --after-context=4 --regexp='Astrometric stats (external)' warning.txt","r");
    while (fgets(line,1000,pip)!=NULL) {
      if(!strncmp(line,"Group",5)) 
	sscanf(line,"%s %s %s %s %s %d %s %s %f ",temp,temp,temp,temp,temp,&nstar,temp,temp,&chi2);  
    }
    pclose(pip);

    system("/bin/cat warning.txt >> log/runscamp.log"); 
    system("/bin/rm warning.txt");
    //system("/bin/cat out.txt");
    //system("/bin/rm out.txt");
    
    printf(" FOR **  %s  ** nstar = %d;   chi2 = %2.3f\n",imarray[i],nstar,chi2); 
    
    /* !!need to check the scamp result here ?? */
    
    /* count how many images in the _fits.list (same # for _bpm.list and _var.list) */
    sprintf(line, "%s/%s/%s_fits.list",dirlocation,imarray[i],imarray[i]);
    flist=fopen(line,"r");
    nimage=0;
    while (fscanf(flist,"%s",temp)!=EOF)
      nimage++;
    fclose(flist);
    if(!flag_quiet) printf(" ** %s has %d images\n",line,nimage);

    /* loop over and insert .head WCS parameter back to im,bpm and var */
    sprintf(headname, "%s/%s/%s_scamp.head",dirlocation,imarray[i],imarray[i]);
    sprintf(outfitslist,"%s/%s/%s_fits.list",dirlocation,imarray[i],imarray[i]);
    sprintf(outbpmlist,"%s/%s/%s_bpm.list",dirlocation,imarray[i],imarray[i]);
    sprintf(outvarlist,"%s/%s/%s_var.list",dirlocation,imarray[i],imarray[i]);

    fhead=fopen(headname,"r");
    ffits=fopen(outfitslist,"r");
    fbpm=fopen(outbpmlist,"r");
    fvar=fopen(outvarlist,"r");

    for(j=0;j<nimage;j++) {

      /* need to initialize */
      fullfits[0]=0; fullbpm[0]=0; fullvar[0]=0; line[0]=0;

      fscanf(ffits,"%s",fullfits);
      fscanf(fbpm,"%s",fullbpm);
      fscanf(fvar,"%s",fullvar);
      
      fouthead=fopen("temp.head","w");
      while(fgets(line,1000,fhead)!=NULL) {
	fprintf(fouthead,"%s",line);
	sscanf(line,"%s",temp);
	if (!strcmp(temp,"END")) 
	  break;
      }
      fclose(fouthead);
      
      /* insert .head keywords back to the image */
      fflush(stdout);
      if(!flag_quiet) {
	printf(" ** Inserting %s file to \n",headname);
	printf("    %s\n",fullfits);
	printf("    %s\n",fullbpm);
	printf("    %s\n",fullvar);
      }

      sprintf(command,"%s/insert_head %s temp.head",binpath,fullfits);
      system(command);
      /*sprintf(command,"%s/insert_head %s temp.head",binpath,fullbpm);
      system(command);
      sprintf(command,"%s/insert_head %s temp.head",binpath,fullvar);
      system(command);
	*/

      system("/bin/rm temp.head");

      /* insert SCAMP results to header */
      status=0;
      if(fits_open_file(&fptr,fullfits,READWRITE,&status)) printerror(status);
      
      if(fits_update_key(fptr,TINT,"SCAMPFLG",&flag_repeat,"Flag for SCAMP; 0=no WARNING; 1=WARNING exists",&status))
	printerror(status);
 
      if(fits_update_key(fptr,TINT,"SCAMPNUM",&nstar,"number of matched stars from SCAMP",&status))
	printerror(status);

      if(fits_update_key(fptr,TFLOAT,"SCAMPCHI",&chi2,"chi2 value from SCAMP",&status))
	printerror(status);

      if(fits_close_file(fptr,&status)) printerror(status);
      
      if(!flag_quiet) 
	printf(" ** Update header of %s with SCAMPFLG=%d\tSCAMPNUM=%d\tSCAMPCHI=%2.2f\n",fullfits,flag_repeat,nstar,chi2);
    }
    fclose(fhead); fclose(ffits); fclose(fbpm); fclose(fvar); 

    /* outpu the result to astrometryQA.log */
    fprintf(fwlog," For image: %s\n",imarray[i]);
    fprintf(fwlog,"   SCAMPFLG = %d\n",flag_repeat);
    fprintf(fwlog,"   SCAMPCHI = %2.4f\n",chi2);
    fprintf(fwlog,"   SCAMPNUM = %d\n",nstar);
    if(flag_warningmssg==1)
      fprintf(fwlog,"   WARNING: Not enough matched detections in set\n");
    if(flag_warningmssg==2)
      fprintf(fwlog,"   WARNING: Significant inaccuracy likely to occur in projection\n");
    fprintf(fwlog,"\n");

    /* move the .ps plots, hardwired at the moment */
    sprintf(command, "/bin/mv %s/fgroups_1.ps %s/%s/%s_fgroups.ps",dirbase,dirlocation,imarray[i],imarray[i]);
    sprintf(command, "%s/bin/mv %s/distortion_1.ps %s/%s/%s_distortion.ps\n",command,dirbase,dirlocation,imarray[i],imarray[i]);
    sprintf(command, "%s/bin/mv %s/astr_referror2d_1.ps %s/%s/%s_astr_referror2d.ps\n",command,dirbase,dirlocation,imarray[i],imarray[i]);
    sprintf(command, "%s/bin/mv %s/astr_referror1d_1.ps %s/%s/%s_astr_referror1d.ps\n",command,dirbase,dirlocation,imarray[i],imarray[i]);
    sprintf(command, "%s/bin/mv %s/astr_pixerror1d_1.ps %s/%s/%s_astr_pixerror1d.ps\n",command,dirbase,dirlocation,imarray[i],imarray[i]);
    sprintf(command, "%s/bin/mv %s/astr_subpixerror1d_1.ps %s/%s/%s_astr_subpixerror1d.ps\n",command,dirbase,dirlocation,imarray[i],imarray[i]);
    sprintf(command, "%s/bin/mv %s/astr_refsysmap_1.ps %s/%s/%s_astr_refsysmap.ps\n",command,dirbase,dirlocation,imarray[i],imarray[i]);
    sprintf(command, "%s/bin/mv %s/shear_vs_airmass_1.ps %s/%s/%s_shear_vs_airmass.ps\n",command,dirbase,dirlocation,imarray[i],imarray[i]);
    system(command);
  }

  fclose(fwlog);

  free(ccdarray); free(impatharray); free(imarray); free(bandarray);
  free(match_resolgrid); free(position_maxerrgrid);

  if(!flag_quiet)
    printf(" ** %s done on %s \n",argv[0],asctime(localtime (&curtime)));
  
  return (0);
}

#undef IMMAX
#undef DECAM
#undef MOSAIC

