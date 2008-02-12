/* subroutines needed for accessing files within the archive */
/* this program requires that the filename include the full directory 
/* tree within the archive
*/

#define NIMCLASS 4
#define RAW 0
#define RED 1
#define CAL 2
#define COADD 3

void filename_resolve(filename,imageclass,runiddesc,nite,tilename,
  imagetype,imagename,band,ccdnumber)
  char filename[],runiddesc[],nite[],band[],tilename[],imagetype[],
    imagename[],imageclass[];
  int *ccdnumber;

/*void filename_resolve(filename,runiddesc,nite,band,tilename,
/*		      imagetype,imagename,ccd_number)
/*    char filename[],runiddesc[],nite[],band[],tilename[],imagetype[],imagename[];
/*    int *ccd_number;
*/
{
        char	image[800],root[800],tmp[800],type[100],
	        imclass[NIMCLASS][10],tag[10][1000];
	int	nslash,i,len,flag_cat=0,flag_raw=0,flag_reduced=0,
		flag_coadd=0,flag_src=0,flag_remap=0,len2,
		nimclass,ccd_number=0;

	sprintf(imclass[RAW],"raw");
	sprintf(imclass[RED],"red");
	sprintf(imclass[CAL],"cal");
	sprintf(imclass[COADD],"coadd");
	
	/* set defaults/clear strings */
	ccd_number=0;
	runiddesc[0]=nite[0]=band[0]=tilename[0]=imagetype[0]=imagename[0]=imageclass[0]=0;

        sprintf(root,"%s",filename);
        len=strlen(root);
	  
	/* look for Archive and remove, else assume filename begins with <imageclass> */
	for (i=0;i<len;i++) 
	  if (!strncmp(root+i,"Archive/",8)) {
	    sprintf(root,"%s",filename+8+i);
	    break;
	  }
	/* confirm that the string begins with one of the imageclasses */
	nimclass=-1;for (i=0;i<NIMCLASS;i++) 
	  if (!strncmp(root,imclass[i],strlen(imclass[i]))) {
	    nimclass=i;
	    sprintf(imageclass,"%s",imclass[i]);
	    break;
	  }
	if (nimclass==-1) {
	  printf("  ** Imageclass (raw, red, coadd, cal) not found: %s\n",
	    filename);
	  exit(0);
	}

        /* replace "/" with spaces and remove _im.fit .fit, .fits or .fits.gz */
        nslash=0;len=strlen(root)-1;
        for (i=len;i>=0;i--) {
	  /* remove .fit* from end */
          if (!strncmp(root+i,".fit",4)) root[i]=0;
	  if (!strncmp(root+i,".fits",5)) root[i]=0;
	  if (!strncmp(root+i,"_im",3)) root[i]=0;
	 
	  /* choong's addition -- changed now by Joe */
	  //if (!strncmp(root+i,"_im.fit",7)) root[i]=0;
	  /* end choong's addition */

          if (!strncmp(root+i,"/",1)) {
            root[i]=32;
	    /* copy the tag we've just found */
	    sscanf(root+i+1,"%s",tag[nslash]);
	    nslash++;
          }
        }

	switch (nimclass) {
	  case RAW: 
	    if (nslash!=4 && nslash!=3) {
	      printf("  ** Filename has wrong number of elements: %s\n",filename);
	      exit(0);
	    }
	    /* log and src data */
	    if (nslash==3) {
	      sprintf(nite,"%s",tag[2]);
	      sprintf(imagetype,"%s",tag[1]);
	      sprintf(imagename,"%s",tag[0]);
	    }
	    /* raw data */
	    if (nslash==4) {
	      sprintf(nite,"%s",tag[3]);
	      sprintf(imagename,"%s",tag[1]);
	      sscanf(&(tag[0][strlen(imagename)+1]),"%d",&ccd_number);	      
	    }
	    break; 
	  case RED:
	    if (nslash!=6 && nslash!=5) {
	      printf("  ** Filename has wrong number of elements: %s\n",filename);
	      exit(0);
	    }
	    sprintf(runiddesc,"%s",tag[nslash-1]);
	    sprintf(nite,"%s",tag[nslash-3]);
	    /* next element is band if not raw, cal or log */
	    if (!strcmp(tag[nslash-4],"raw") && !strcmp(tag[nslash-4],"cal") &&
	      !strcmp(tag[nslash-4],"log")) sprintf(band,"%s",tag[nslash-4]);
	    sprintf(imagename,"%s",tag[nslash-5]);
	    if (!strcmp(tag[nslash-4],"log")) {
	      sprintf(imagetype,"log");
	      break;
	    }
	    /* only cal, raw and <band> beyond here */
	    sprintf(imagename,"%s",tag[1]);
	    /* strip band out of imagename if this is cal image */
	    if (!strcmp(tag[2],"cal")) {
	      len=strlen(imagename);
	      for (i=len;i>=0;i--) if (!strncmp(imagename+i,"_",1)) {
	        sprintf(band,"%s",&(tag[1][i+1]));
		tag[1][i]=0;
	      }
	      sprintf(imagetype,"%s",tag[1]);
	    }
	    if (!strcmp(tag[2],"cal") || !strcmp(tag[2],"raw"))
	      sscanf(&(tag[0][strlen(imagename)+1]),"%d",&ccd_number);
	    else { /* this is a <band> directory */
	      sprintf(band,"%s",tag[2]);
	      /* check for _cat ending */
	      len=strlen(tag[0]);
	      if (!strncmp(&(tag[0][len-4]),"_cat",4)) {
		sprintf(imagetype,"catalog");
		tag[0][len-4]=0;
	      }
	      /* check for _## ending */
	      len=strlen(tag[0]);
	      if (!strncmp(&(tag[0][len-3]),"_",1)) {
		sscanf(&(tag[0][len-2]),"%d",&ccd_number);
		tag[0][len-3]=0;
	        if (strcmp(imagetype,"catalog")) sprintf(imagetype,"reduced");
	      }
	      else { /* must have tilename, too */
	        len=strlen(tag[0]);
	        for (i=len-1;i>=0;i--) {
	          if (!strncmp(&(tag[0][i]),".",1)) {
		    /* grab tilename */
		    sprintf(tilename,"%s",&(tag[0][i+1]));
		    tag[0][i]=0; 

		    /* choong's change 12/05/2007 */
		    break;
		    /* grab ccd_number */
		    //sscanf(&(tag[0][i-2]),"%d",&ccd_number);
		  }
	        }
		sscanf(&(tag[0][i-2]),"%d",&ccd_number);
		/* end choong's change 12/05/2007 */
	        if (strcmp(imagetype,"catalog")) sprintf(imagetype,"remap");
	      }	      
	    } 
	    break;
	  case COADD:
	    if (nslash!=3) {
	      printf("  ** Filename has wrong number of elements: %s\n",filename);
	      exit(0);
	    }
	    sprintf(runiddesc,"%s",tag[2]);
	    sprintf(tilename,"%s",tag[1]);
	    sprintf(imagename,"%s",tag[0]);
	    if (!strncmp(&(tag[0][strlen(tag[0])-4]),"_cat",4)) {
	      sprintf(imagetype,"catalog");
	      tag[0][strlen(tag[0])-4]=0;
	    }
	    else sprintf(imagetype,"coadd");
	    sprintf(band,"%s",&(tag[0][strlen(tilename)+1]));
	    break;
	  case CAL:
	    if (nslash!=4) {
	      printf("  ** Filename of wrong number of elements: %s\n",filename);
	      exit(0);
	    }
	    sprintf(imagetype,"%s",tag[3]);
	    sprintf(nite,"%s",tag[2]);
	    sprintf(imagename,"%s",tag[1]);
	    //sscanf(&(tag[0][strlen(imagename)+1]),"%d",ccd_number); 
	}

	*ccdnumber=ccd_number;
}


void filename_construct(filename,imageclass,runiddesc,nite,tilename,
	imagetype,imagename,band,ccd_number)
	char filename[],runiddesc[],nite[],band[],tilename[],imagetype[],
	  imagename[],imageclass[];
	int ccd_number;
{
	char	image[800],root[800],tmp[800],type[100],
		imclass[NIMCLASS][10],
		tag[10][1000];
	int	nslash,i,len,flag_cat=0,flag_raw=0,flag_reduced=0,
		flag_coadd=0,flag_src=0,flag_remap=0,len2,
		nimclass;
	
	sprintf(imclass[RAW],"raw");
	sprintf(imclass[RED],"red");
	sprintf(imclass[CAL],"cal");
	sprintf(imclass[COADD],"coadd");
	
	nimclass=-1;for (i=0;i<NIMCLASS;i++)
	  if (!strncmp(imageclass,imclass[i],strlen(imclass[i]))) {
	    nimclass=i;
	    break;
	  }
	if (nimclass==-1) {
	  printf("  ** Imageclass not defined (raw, red, coadd, cal): %s\n",
	    imageclass);
	  exit(0);
	}
	
	switch (nimclass) {
	/********************************************************************/
	/********************       RAW DATA          ***********************/
	/********************************************************************/	
	  case RAW:
	    if (!strcmp(imagetype,"src"))
	      sprintf(filename,"%s/%s/%s/%s.fits",imageclass,nite,imagetype,
	        imagename);
	    else if (!strcmp(imagetype,"log"))	      
	      sprintf(filename,"%s/%s/%s/%s",imageclass,nite,imagetype,
	        imagename);
	    else {
	      sprintf(filename,"%s/%s/raw/%s/%s_%02d.fits",imageclass,nite,
	        imagename,imagename,ccd_number);	      
	    }
	    break;
	/********************************************************************/
	/********************  CALIBRATION DATA       ***********************/
	/********************************************************************/		  
	  case CAL:
	    sprintf(filename,"%s/%s/%s/%s/%s_%02d.fits",imageclass,
	      imagetype,nite,imagetype,imagetype,ccd_number);
	    break;
	/********************************************************************/
	/********************    COADDED DATA         ***********************/
	/********************************************************************/		  
	  case COADD:
	    if (!strcmp(imagetype,"catalog")) 
	      sprintf(filename,"%s/%s/%s/%s_%s_cat.fits",imageclass,runiddesc,
	        tilename,tilename,band);
	    else 
	      sprintf(filename,"%s/%s/%s/%s_%s.fits",imageclass,runiddesc,
	        tilename,tilename,band);
	    break;
	/********************************************************************/
	/********************    REDUCED DATA         ***********************/
	/********************************************************************/	
	  case RED:
	    /* log data */
	    if (!strcmp(imagetype,"log")) 
	      sprintf(filename,"%s/%s/data/%s/%s/%s",imageclass,runiddesc,
	        nite,imagetype,imagename);
	    /* data types within the <band> directories */
	    else if (!strcmp(imagetype,"reduced"))
	      sprintf(filename,"%s/%s/data/%s/%s/%s/%s_%02d.fits",imageclass,
	        runiddesc,nite,band,imagename,imagename,ccd_number);
	    else if (!strcmp(imagetype,"remap"))
	      sprintf(filename,"%s/%s/data/%s/%s/%s/%s_%02d.%s.fits",imageclass,
	        runiddesc,nite,band,imagename,imagename,ccd_number,tilename);
	    else if (!strcmp(imagetype,"catalog")) {
	      if (strlen(tilename)>0) /* catalog of remap image */
	       sprintf(filename,"%s/%s/data/%s/%s/%s/%s_%02d.%s_cat.fits",
	         imageclass,runiddesc,
	         nite,band,imagename,imagename,ccd_number,tilename);	
	      else sprintf(filename,"%s/%s/data/%s/%s/%s/%s_%02d_cat.fits",
	        imageclass,runiddesc,
	        nite,band,imagename,imagename,ccd_number);    
	    }
	    /* raw data with imageclass="red" */
	    else if (!strcmp(imagetype,"object") || !strcmp(imagetype,"zero") ||
	      !strcmp(imagetype,"bias") || !strcmp(imagetype,"dark") ||
	      !strcmp(imagetype,"dome flat") || !strcmp(imagetype,"sky flat") ||
	      !strcmp(imagetype,"flat"))
	      sprintf(filename,"%s/%s/data/%s/%s/%s/%s_%02d.fits",imageclass,
	        runiddesc,nite,"raw",imagename,imagename,ccd_number);
	    /* calibration data with imageclass="red" */
	    else if (!strcmp(imagetype,"bpm") || !strcmp(imagetype,"biascor") || 
	      !strcmp(imagetype,"zerocor") || !strcmp(imagetype,"darkcor") ||
	      !strcmp(imagetype,"ghost"))
	      sprintf(filename,"%s/%s/data/%s/cal/%s/%s_%02d.fits",imageclass,
	        runiddesc,nite,imagetype,imagetype,ccd_number);
	    else if (!strcmp(imagetype,"flatcor") || !strcmp(imagetype,"illumcor") ||
	      !strcmp(imagetype,"supersky") || !strcmp(imagetype,"fringecor"))
	      sprintf(filename,"%s/%s/data/%s/cal/%s_%s/%s_%s_%02d.fits",imageclass,
	        runiddesc,nite,imagetype,band,imagetype,band,ccd_number);
	    else {
	      printf("  ** Imagetype %s within Imageclass \"red\" not found\n",
	        imagetype);
	      exit(0);
	    }
	    break;
		
	}
}

void splitstring(line,separator,num_fields,len_field,results)
	char results[],line[],separator[];
	int  num_fields,len_field;
{
	int	current,base,len,j,lensep;

	len=strlen(line);
	lensep=strlen(separator); 
	if (lensep>1) {
	  printf("  **splitstring:  separator must be length 1\n");
	  exit(0);
	}
	
	base=current=0;
	for (j=0;j<len;j++) {
	  if (strncmp(line+j,separator,1)) 
	    results[current*len_field+j-base]=line[j];
	  else { /* reset for next string */
	    /* end current result string */
	    results[current*len_field+j-base]=0;
	    base=j+1;
	    current++;
	    if (current>num_fields) {
	      printf("  **splitstring:  more than %d fields- %s\n");
	      exit(0);
	    }
	  }
	}
}



#undef NIMCLASS
#undef RED
#undef RAW
#undef CAL
#undef COADD
