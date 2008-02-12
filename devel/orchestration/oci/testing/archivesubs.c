/* subroutines needed for accessing files within the archive */

void filename_resolve(filename,runid,nite,band,tilename,imagetype,imagename,
		ccdnum)
	char filename[],runid[],nite[],band[],tilename[],imagetype[],
	  imagename[];
	int *ccdnum;
{

	char	image[800],root[800],tmp[800],type[100];
	int	nslash,i,len,flag_cat=0,flag_raw=0,flag_reduced=0,
		flag_coadd=0,flag_src=0,flag_remap=0,len2;

	/* set defaults */
	*ccdnum=0;

          /* use imagename to extract the nite and actual name*/
          /* assume form:  $runid/data/$nite/$type/imname/imname_##.fits */
          /* so look between the 3rd and 4th "/" from the end */

          /* for remap image, the string is assumed to be */
          /* $runid/data/$nite/$type/imname/imname_##.tilename.fits */

          sprintf(root,"%s",filename);
          len=strlen(root);

          /* replace "/" with spaces and remove .fits */
          nslash=0;

          for (i=len-1;i>=0;i--) {
            if (!strncmp(root+i,".fit",4)) root[i]=0;
            if (!strncmp(root+i,"/",1)) {
              root[i]=32;
              nslash++;
              if (nslash==6) break;
            }
          }
	  if (i<0) i=0;


	  if ((nslash<4 && strncmp(root,"raw",3)) || (nslash<3 && !strncmp(root,"raw",3))) {
	    printf("Not enough fields defined: %s\n",root);
	    exit(0);
	  }
	
          /* read in info */
          sscanf(root+i,"%s %s %s %s %s %s",
	    runid,tmp,nite,band,imagename,image);

          /* find out what kind of file */
	  /* is it raw data? */
          if (!strncmp(type,"raw",3)) {
	    flag_raw=1;
	    sprintf(imagetype,"raw");
	    band[0]=0; /* clear band information*/
	    runid[0]=0; /* clear runid information*/
	  }

	  /* is it src data? */
          if (!strncmp(band,"src",3)) {
	    flag_src=1;
	    sprintf(imagetype,"src");
	    band[0]=0; /* clear band information*/
	    runid[0]=0; /* clear runid information*/
	  }

	  /* make tmp the suffixes */
	  len2=strlen(imagename);
	  sprintf(tmp,"%s",image+len2);
	  len=strlen(tmp);

	  /* is it a catalog? */
          if(!strncmp(&tmp[len-4],"_cat",4)) {
	    flag_cat=1;
	    sprintf(imagetype,"catalog");
	    /* trim off the catalog suffix */
	    tmp[len-4]=0;
	    len=strlen(tmp);
	  }

	  /* now examine suffixes */
	  for (i=0;i<len;i++) 
	    if (!strncmp(tmp+i,".",1)) tmp[i]=32;
	  /* look for ccdnum */
	  for (i=0;i<len;i++) {
	    if (!strncmp(tmp+i,"_",1)) {
	      sscanf(tmp+1+i,"%d",ccdnum);
	    }
	    if (!strncmp(tmp+i," ",1)) {
	      sscanf(tmp+i+1,"%s",tilename);
	      flag_remap=1;
	    }
	  }
	  if (!flag_remap)  {
	    flag_reduced=1;
	    tilename[0]=0;
	  }

	if (flag_cat) sprintf(imagetype,"catalog");
	else {
	  if (flag_reduced) sprintf(imagetype,"reduced");
	  if (flag_remap) sprintf(imagetype,"remap");
	  if (flag_raw) sprintf(imagetype,"raw");
	  if (flag_src) sprintf(imagetype,"src");
	  if (flag_coadd) sprintf(imagetype,"coadd");
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

