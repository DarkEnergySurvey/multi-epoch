/* run at this level:
  
../uid/data/nite/band

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXLISTS 50
#define MAXIMAGE 100

main(argc,argv)
	int argc;
	char *argv[];
{
	char	imagename[800],command[15000],
		imagenamesave[800],cleanup[20],
		filelist[MAXLISTS][350],
		inputimage[MAXIMAGE][800],binpath[800];
	int	j,i,len,imnum=0,flag_quiet=0, 
		nlists=0,flag_cat=0,flag_cleanup=0,flag_bin=0;
	FILE	*fin,*pip;

  	if ( argc < 3 ) {
    	  printf ("Usage: %s <list1> <list2> ... <listN>\n", 
	    argv [0]);
	  printf("  Options:\n");
	  printf("  -bin <dir>\n");
	  printf("  -cleanup\n");
	  printf("  -quiet\n");
    	  exit(0);
  	}
	
	sprintf(binpath, "%s", argv[argc-1]);
	
	/* process the command line */
	for (i=1;i<argc;i++) {
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	  else {
	    if (!strcmp(argv[i],"-cleanup")) flag_cleanup=1;
	    else {  
	      if (!strcmp(argv[i],"-bin")) {
	  	flag_bin=1;
	 	i++;
		/* read bin */
		sscanf(argv[i],"%s",binpath);
	      }
	      else {
	        /* assume it is a file list */
	        sprintf(filelist[nlists],"%s",argv[i]);
	        /* check list file to confirm it exists */
	        if ( (fin=fopen (filelist[nlists], "r")) == NULL ) {
	          printf ("  ** runFitscombine:  List file \"%s\" not found.\n", 
		  filelist[nlists]);
	    	  exit (0);
	        }  
	        else { /* confirm that it contains FITS files */
	          while (fscanf(fin,"%s",imagename)!=EOF) {
	            if (strncmp(&(imagename[strlen(imagename)-5]),".fits",
		    5)) {
	              printf("  **runFitscombine: File must contain list of FITS images **\n");
	              exit(0);
	            }
	          }
	          fclose(fin);
	        }
	        nlists++;
	        if (nlists>=MAXLISTS) {
		  printf("  **runFitscombine:  MAXLISTS exceeded!\n");
		  exit(0);
	        }
	      }
	    }
	  }
	}
	if (flag_cleanup) sprintf(cleanup,"-cleanup");
	else cleanup[0]=0; /* clear this string */
	if (!flag_bin) binpath[0]=0;

	/* now cycle through lists to fitscombine the files contained */
	for (j=0;j<nlists;j++) {
	  fin=fopen(filelist[j],"r");
	  if(!flag_quiet) printf("\n ** Processing list %s\n", 
	    filelist[j]);
	  /* read through image list */
	  while (fscanf(fin,"%s",imagename)!=EOF) {
	    flag_cat=0;
	    sprintf(imagenamesave,"%s",imagename);
	    /* remove .fits from imagename*/
	    imagename[strlen(imagename)-5]=0;
	    /* is this a catalog? */
	    if (!strncmp(imagename+strlen(imagename)-4,"_cat",3)) {
	      flag_cat=1;
	      /* strip off the _cat */
	      imagename[strlen(imagename)-4]=0;
	    }
	    /* probe for files to combine */
	    imnum=0;
	    sprintf(command,"ls %s_*.fits",imagename);
	    pip=popen(command,"r");
	    while (fscanf(pip,"%s",inputimage[imnum])!=EOF) {
	      len=strlen(inputimage[imnum]);
	      if (flag_cat) { /* accept only _cat, _obj or _reg */
	        if (!strncmp(inputimage[imnum]+len-9,"_tab.fits",9) ||
	          !strncmp(inputimage[imnum]+len-9,"_reg.fits",9) ||
	          !strncmp(inputimage[imnum]+len-9,"_obj.fits",9))
		  imnum++;
	      }
	      else { /* assume its an image of some sort */
	        if (!strncmp(inputimage[imnum]+len-8,"_im.fits",8) ||
	          !strncmp(inputimage[imnum]+len-9,"_var.fits",9) ||
	          !strncmp(inputimage[imnum]+len-9,"_bpm.fits",9))
		  imnum++;
	      }
	      if (imnum>=MAXIMAGE) {
	        printf("  ** runFitscombine:  Exceeded MAXIMAGE in %s\n",
		  command);
	        exit(0);
	      }
	    }
	    pclose(pip);
	    if (imnum==0) {
	      printf("  ** runFitscombine:  found no images for %s\n",
		command);
	    } 
	    command[0]=0; /* clear the last command */
	    if (imnum==1 && !flag_cat) /* simply rename image file */
	      sprintf(command,"mv %s %s",inputimage[0],imagenamesave);
	    if (imnum==3 && flag_cat) { /* tab, reg, obj case */
	      /* combine the input images into a single output file */
	      sprintf(command, "%s/fitscombine %s %s_tab.fits %s_reg.fits %s_obj.fits %s", 
	        binpath,cleanup,imagename,imagename,imagename,imagenamesave);
	    }
	    if (imnum==2 && !flag_cat) { /* im, var case */
	      /* combine the input images into a single output file */
	      sprintf(command, "%s/fitscombine %s %s_im.fits %s_var.fits %s", 
	 	binpath,cleanup,imagename,imagename,imagenamesave);
	    }
	    if (imnum==3 && !flag_cat) {  /* im, var, bpm case */
	      /* combine the input images into a single output file */
	      sprintf(command, "%s/fitscombine %s %s_im.fits %s_var.fits %s_bpm.fits %s", 
	 	binpath,cleanup,imagename,imagename,imagename,
		imagenamesave);
	    }
	    if (strlen(command)) {
	      if (!flag_quiet) printf("%s\n",command);
	      system (command);
	    }
	  } /* completed processing of a single image */
	  fclose(fin);
	} /* completed processing of a single list */

	return(0);
}

#undef MAXLISTS
#undef MAXIMAGE
