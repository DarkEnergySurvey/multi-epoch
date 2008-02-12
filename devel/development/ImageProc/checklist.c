/* 
   Checking if two input lists are the same or not

   run at this level:
  
   ../uid/data/nite/band
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

main(argc,argv)
        int argc;
        char *argv[];
{
  char   listname1[800], listname2[800], binpath[1200], nite[500], band[10], project[500];
  char   command[1500], command1[1500], imagename[800],imagename2[800], tilename[800], tempname[800];
  char   line[5000];
  char   **imagename1;
  int    i, j, len, imnum1=0,imnum2=0,imm,flag_quiet=0,flag_remap=0,flag_qcheck=0,flag_im=0,check,checkrm;
  FILE   *in1,*pip,*in2;

  if (argc<2) {
    printf("Usage: %s <product_list> <original_list> \n", argv[0]);
    printf("       Options:\n");
    //printf("        -remap <binpath> <nite> <band> <project> \n");
    printf("        -quick_check \n");
    printf("        -im (if product image names look like *_im.fits) \n");
    printf("        -quiet \n");
    exit(0);
  }
  
  /* process the command line */
  for (i=3;i<argc;i++) {
    if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
    if (!strcmp(argv[i],"-quick_check")) flag_qcheck=1;
    if (!strcmp(argv[i],"-im")) flag_im=1;
    if (!strcmp(argv[i],"-remap")) 
      {
	flag_remap=1;
	sprintf(binpath, "%s", argv[i+1]);
	sprintf(nite, "%s", argv[i+2]);
	sprintf(band, "%s", argv[i+3]);
	sprintf(project, "%s", argv[i+4]);
      }
  }
  
  sprintf(listname1, "%s", argv[1]);
  sprintf(listname2, "%s", argv[2]);
  
  /* check the first list */
  imnum1=0;
  in1=fopen(listname1,"r");
  while (fscanf(in1,"%s",imagename)!=EOF) {
    imnum1++;
    if (strncmp(&(imagename[strlen(imagename)-5]),".fits",5) || strncmp(&(imagename[strlen(imagename)-8]),".fits.gz",8)) {
      printf("  ** File must contain list of .fits or .fits.gz images **\n");
      exit(0);
    }
  }
  fclose(in1);

  /* check the second list */
  imnum2=0;
  in2=fopen(listname2,"r");
  while (fscanf(in2,"%s",imagename)!=EOF) {
    imnum2++;
    if (strncmp(&(imagename[strlen(imagename)-5]),".fits",5) || strncmp(&(imagename[strlen(imagename)-8]),".fits.gz",8)) {
      printf("  ** File must contain list of .fits or .fits.gz images **\n");
      exit(0);
    }
  }
  fclose(in2);

  /* quick check if imnum1 = imnum2 */
  if(flag_qcheck) {
    if(imnum1==imnum2) {
      printf(" ** Two lists have the same number of images\n");
      exit(0);
    }
    else {
      printf(" ** There are missing or additional (remap) images, re-run %s without -quick_check option\n",argv[0]);
      exit(0);
    }
  }
  
  /* memory allocation for the first list */
  imagename1=(char **)calloc(imnum1,sizeof(char *));
  for(i=0;i<imnum1;i++) 
     imagename1[i]=(char *)calloc(800,sizeof(char));
 
  /* store first list info, first list is the product list */ 
  in1=fopen(listname1,"r");
  for (imm=0;imm<imnum1;imm++) 
    fscanf(in1,"%s",imagename1[imm]);
  fclose(in1);
   
  /* now check the second list against the first list, second list is the original list */
  in2=fopen(listname2,"r");
  for (imm=0;imm<imnum2;imm++) {
    
    fscanf(in2,"%s",imagename);
    
    /* now cycle through first list */
    check=0;
    checkrm=0;
    for(i=0;i<imnum1;i++) {

      if(!flag_remap) { /* not remap images */
	if(flag_im) {
	  len=strlen(imagename);
	  for (j=len;j>0;j--) if (!strncmp(&(imagename[j]),".fits",5)) {
	    imagename[j]=0;
	    break;
	  }
	  sprintf(imagename2,"%s_im.fits",imagename);
	}
	else
	  sprintf(imagename2,"%s",imagename);
     
	if(strcmp(imagename1[i],imagename2)==0) { 
	  check=1; break;
	}
      }
      else { /* remap images */
	
	/* get the root name without .fits */
	len=strlen(imagename);
	for (j=len;j>0;j--) if (!strncmp(&(imagename[j]),".fits",5)) {
	  imagename[j]=0;
	  break;
	}
	sprintf(tempname,"%s",imagename);
	
	/* now get the tilename */
	sprintf(command, "%s/findtile %s %s %s %s -quiet -tilename_only", binpath, imagename, nite, band, project);

	pip=popen(command,"r");
	while (fgets(tilename,5000,pip)!=NULL) {
	  
	  if(flag_im) 
	    sprintf(imagename2,"%s.%s_im.fits",tempname,tilename);
	  else
	    sprintf(imagename2,"%s.%s.fits",tempname,tilename);
	  
	  if(strcmp(imagename1[i],imagename2)==0) { 
	    check=1; checkrm=1; break;
	  }
	}
	pclose(pip); 
	//if(checkrm == 1) break;
      }

    } /* i loop */

    if(!check) printf(" ** Missing %s in %s\n",imagename2,listname1);
  }
  fclose(in2);
    
  /* free memory */
  free(imagename1);

  return(0);
}
  
