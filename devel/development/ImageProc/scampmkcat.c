/* 
    now make the -runid compulsary, otherwise will have problem when query the database with different runid that has different ccd_number
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define DECAM 62
#define MOSAIC 8

main(int argc, char *argv[])

{
  char runid[1000],nite[1000],outlocation[1000],line1[1000],stdname[1000];
  char sqlqueryfile[1000],sqlcall[1000],fileroot[1000],stdroot[1000];
  char filename[1000],runiddesc[1000],band[1000],imagename[1000],filelinetemp[1000];
  char fileline1[1000],ccd_string[1000],tempnite[1000],tempband[1000];
  char tempimagename[1000],temprunid[1000],tempccd_number[1000];
  char fileline2[1000],runid2[1000],ccd_string2[1000],band2[1000];
  char imagename2[1000],sqlqueryfile2[1000],line3[1000],bandselect[10];
  char project[10],prev_imname[1000],impath[1200],runid_in[1000];
  char	dblogin[500];

  int i,j,numlists,flag_quiet=0,flag_runid=0,flag_stdlist=0,flag_noparallel=0,ccd_number,len;
  int flag_index1,flag_index2,flag_index3,flag_match;
  int counter,index1[100000],counter2,len2;
  int ccd_number2,flag_index21,flag_index22,flag_index23;
  int k,l,distinct_number,distinct_g,distinct_r,distinct_i,distinct_z;
  int loop,Ntotal,Nexposure,threshold,count;

  FILE *out,*pip,*fileout,*filein,*filein2,*fileout2,**filemain,**stdmain,*filetemp,*infile;
  void select_dblogin();
  
  /* see if the correct number of command line arguments are available */
  
  if(argc<5) {
    printf("Usage: %s <nite> <output_root> <# lists> <project> -runid <runid>\n",argv[0]);
    printf("        Options:\n");
    printf("              -standardlist <standardlist_root>\n");
    printf("              -noparallel\n");
    printf("              -quiet\n");
    exit(0);
  } 

  sprintf(nite,"%s",argv[1]);
  sprintf(fileroot,"%s",argv[2]);
  numlists=atoi(argv[3]);
  sprintf(project,"%s",argv[4]);
  
  for (i=5;i<argc;i++) {
    if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
    if (!strcmp(argv[i],"-noparallel")) flag_noparallel=1;
    if (!strcmp(argv[i],"-standardlist")) {
      flag_stdlist=1;
      sprintf(stdroot,"%s",argv[i+1]);
    }
    if (!strcmp(argv[i],"-runid")) 
      { 
	flag_runid=1;
	sprintf(runid_in,"%s",argv[i+1]);
      }
  }

  /* determine db login information*/
  select_dblogin(dblogin);
  /* build Database query                          */
  sprintf(sqlqueryfile,"mkscampcatlist.sqlquery");
  sprintf(sqlcall,"${ORACLE_HOME}/bin/sqlplus -S %s < %s",dblogin,sqlqueryfile);

  /* prepare query */
  out=fopen(sqlqueryfile,"w");
  fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP ' '");
  fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
  fprintf(out,"SELECT runiddesc||' '||band||' '||imagename||' '||nite||' '||ccd_number FROM FILES\n");
  fprintf(out,"WHERE nite='%s' and imagetype='reduced' ",nite);
  if(flag_runid)
    fprintf(out,"and runiddesc like '%s\%' \n ",runid_in);
  //fprintf(out,"order by ra;\n");
  fprintf(out,"order by imagename;\n");
  fprintf(out,"exit;\n");
  fclose(out);
  /* output pathnames of chip catalogs to a list */

  fileout=fopen("sql_list","w");
  pip=popen(sqlcall,"r");
  
  while (fgets(line1,1000,pip)!=NULL) {
    sscanf(line1,"%s %s %s %s %d",runiddesc,band,imagename,nite,&ccd_number);
    fprintf(fileout,"%s/data/%s/%s/%s/%s_%02d\n",runiddesc,nite,band,imagename,imagename,ccd_number);
    if (!flag_quiet) {
      line1[strlen(line1)-1]=0;
      //fprintf(stdout,"%s     %s\n",line1,fileroot);
    }
  }
  pclose(pip);
  fclose(fileout);
  
  /* query the database to find distinct runids in order to prevent unnecessary looping */
  /* loop over all filters   */

  for (l=0; l<4; l++){
    
    if (l==0) strcpy(bandselect,"g");
    if (l==1) strcpy(bandselect,"r");
    if (l==2) strcpy(bandselect,"i");
    if (l==3) strcpy(bandselect,"z");
    
    sprintf(sqlqueryfile2,"mkscampcatlist.sqlquery2");
    sprintf(sqlcall,"${ORACLE_HOME}/bin/sqlplus -S %s < %s",dblogin,
	sqlqueryfile2);
    
    /* prepare 2nd query */
    out=fopen(sqlqueryfile2,"w");
    fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP ' '");
    fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
    fprintf(out,"SELECT count(distinct(runiddesc)) FROM FILES\n");
    fprintf(out,"WHERE nite='%s' and imagetype='reduced' and band='%s' ",nite,bandselect);
    if(flag_runid)
      fprintf(out,"and runiddesc like '%s\%' \n ",runid_in);
    //fprintf(out,"order by ra;\n");
    fprintf(out,"order by imagename;\n");
    fprintf(out,"exit;\n");
    fclose(out);
    pip=popen(sqlcall,"r");
    
    /* temporary solution for DES case, assume only one runid */ 
    while (fgets(line3,1000,pip)!=NULL) {
      sscanf(line3,"%d",&distinct_number);
      if (l==0) {
	if (!strcmp(project,"DES")) { 
	  if(distinct_number <= DECAM) distinct_g=1;
	  else distinct_g=distinct_number;
	}
	else distinct_g=distinct_number;
      }

      if (l==1) {
	if (!strcmp(project,"DES")) {
	  if(distinct_number <= DECAM) distinct_r=1;
	  else distinct_r=distinct_number;
	}
	else  distinct_r=distinct_number;
      }

      if (l==2) {
	if (!strcmp(project,"DES")) {
	  if(distinct_number <= DECAM) distinct_i=1;
	  else distinct_i=distinct_number;
	}
	else  distinct_i=distinct_number;
      }

      if (l==3) {
	if (!strcmp(project,"DES")) {
	  if(distinct_number <= DECAM) distinct_z=1;
	  else distinct_z=distinct_number;
	}
	else distinct_z=distinct_number;
      }
    }
    pclose(pip);
  }
 
  /* open the list file and build lists with imagenames and catalog location */
  
  filein=fopen("sql_list","r");
  filetemp=fopen("temp.dat","w");
  
  /********************************************************************************************/
  /* START OF MAIN LOOP READ                 */
  /********************************************************************************************/
  
  
  /* initialize variables    */
  /*for (i=0; i < 1000; i++) index1[i]=0;*/
  counter=0;
  
  loop=0;
  while (fscanf(filein,"%s",fileline1)!=EOF) {
    
    counter++;  

    /********************************************************************************/
    /* break the file lines into individual variables  */
    len=strlen(fileline1);
    sprintf(filelinetemp,"%s",fileline1);
    for (j=0;j<len;j++) 
      if (!strncmp(&(filelinetemp[j]),"_",1)) {
	filelinetemp[j]=0;
	break;
      }
    sprintf(runid,"%s_",filelinetemp);

    //strncpy(runid,fileline1,12);
    substr(ccd_string,fileline1,len-2,2); 
    ccd_number=atoi(ccd_string);
    
    flag_index1=0;
    flag_index2=0;
    flag_index3=0;
    
    for (i=0;i<len+1;i++) {
      if (!strncmp(&(fileline1[i]),"/",1)) {
	flag_index1++;
	if(flag_index1==3){
	  substr(band,fileline1, i+1, 1);    
	}
	if(flag_index1==4){
	  flag_index2=i;
	}
	if(flag_index1==5){
	  flag_index3=i-1;
	  substr(imagename,fileline1, i+1,flag_index3-flag_index2);
	}
	
      }
    }
    /*   end of variable assignment for a given line read from the file */
    /***********************************************************************************/
    
    /* read the variables into a temp location */
    
    sprintf(tempnite,"%s",nite);
    sprintf(tempband,"%s",band);
    sprintf(tempimagename,"%s",imagename);
    sprintf(temprunid,"%s",runid);
    sprintf(tempccd_number,"%s",ccd_string);
    
    /* open up second version of the input file for looping through looking for later runids */
    counter2=0;
    filein2=fopen("sql_list","r");
    flag_match=0;
    while (fscanf(filein2,"%s",fileline2)!=EOF) {
      counter2++;  
      /* break the file lines into individual variables  */
      len2=strlen(fileline2);
      sprintf(filelinetemp,"%s",fileline2);
      for (j=0;j<len;j++) 
	if (!strncmp(&(filelinetemp[j]),"_",1)) {
	  filelinetemp[j]=0;
	  break;
	}
      sprintf(runid2,"%s_",filelinetemp);

      //strncpy(runid2,fileline2,12); 
      substr(ccd_string2,fileline2,len2-2,2); 
      ccd_number2=atoi(ccd_string2);
      flag_index21=0;
      flag_index22=0;
      flag_index23=0;
      
      for (k=0;k<len2+1;k++) {
	if (!strncmp(&(fileline2[k]),"/",1)) {
	  flag_index21++;
	  if(flag_index21==3){
	    substr(band2,fileline2,k+1,1);    
	  }
	  
	  if(flag_index21==4){
	    flag_index22=k;
	  }
	  
	  if(flag_index21==5){
	    flag_index23=k-1;
	    substr(imagename2,fileline2,k+1,flag_index23-flag_index22); 
	  }
	}
      }
      
      /* look for later runids for each bands; make sure we are not looking at the same line in file */    
		 
      if ((!strcmp(tempband,"g")) && (distinct_g==1)) {
	flag_match=1;
	break;
      }
      if ((!strcmp(tempband,"g")) && (distinct_g>1)){
	
	/* loop through list looking for later-time runiddesc      */
	/* stop loop if imagename changes since imagenames are ordered   */
		
	//if((counter!=counter2) && (strcmp(nite,tempnite)==0) && (strcmp(band2,tempband)==0) && (strcmp(imagename2,tempimagename)==0) && (strcmp(ccd_string2,tempccd_number)==0) && strcmp(runid2,temprunid)>0){
	if((counter!=counter2) && (strcmp(nite,tempnite)==0) && (strcmp(band2,tempband)==0) && (strcmp(imagename2,tempimagename)==0) && strcmp(runid2,temprunid)>0){
       
	  /* assign a new runid if it is later in time than the current value   */
	  //if(strcmp(runid2,temprunid)>0) {

	  sprintf(temprunid,"%s",runid2);
	  flag_match=1;

	  //}
	}
      }
      
      if ((!strcmp(tempband,"r")) && (distinct_r==1)) {
	flag_match=1;
	break;
      }
      if ((!strcmp(tempband,"r")) && (distinct_r>1)){
	
	/* loop through list looking for later-time runiddesc      */
	/* stop loop if imagename changes since imagenames are ordered   */
	
	//if((counter!=counter2) && (strcmp(nite,tempnite)==0) && (strcmp(band2,tempband)==0) && (strcmp(imagename2,tempimagename)==0) && (strcmp(ccd_string2,tempccd_number)==0) && strcmp(runid2,temprunid)>0){
	if((counter!=counter2) && (strcmp(nite,tempnite)==0) && (strcmp(band2,tempband)==0) && (strcmp(imagename2,tempimagename)==0) && strcmp(runid2,temprunid)>0){
	  
	  /* assign a new runid if it is later in time than the current value   */
	  sprintf(temprunid,"%s",runid2);
	  flag_match=1;
	}
      }
      
      if ((!strcmp(tempband,"i")) && (distinct_i==1)) {
	flag_match=1;
	break;
      }
      if ((!strcmp(tempband,"i")) && (distinct_i>1)){
	
	/* loop through list looking for later-time runiddesc      */
	/* stop loop if imagename changes since imagenames are ordered   */
	
	//if((counter!=counter2) && (strcmp(nite,tempnite)==0) && (strcmp(band2,tempband)==0) && (strcmp(imagename2,tempimagename)==0) && (strcmp(ccd_string2,tempccd_number)==0) && strcmp(runid2,temprunid)>0){
	if((counter!=counter2) && (strcmp(nite,tempnite)==0) && (strcmp(band2,tempband)==0) && (strcmp(imagename2,tempimagename)==0) && strcmp(runid2,temprunid)>0){

	  /* assign a new runid if it is later in time than the current value   */
	  sprintf(temprunid,"%s",runid2);
	  flag_match=1;
	}
      }
      
      if ((!strcmp(tempband,"z")) && (distinct_z==1)) {
	flag_match=1;
	break;
      }
      if ((!strcmp(tempband,"z")) && (distinct_z>1)){
	
	/* loop through list looking for later-time runiddesc      */
	/* stop loop if imagename changes since imagenames are ordered   */
	
	//if((counter!=counter2) && (strcmp(nite,tempnite)==0) && (strcmp(band2,tempband)==0) && (strcmp(imagename2,tempimagename)==0) && (strcmp(ccd_string2,tempccd_number)==0) && strcmp(runid2,temprunid)>0){
	if((counter!=counter2) && (strcmp(nite,tempnite)==0) && (strcmp(band2,tempband)==0) && (strcmp(imagename2,tempimagename)==0) && strcmp(runid2,temprunid)>0){
	  
	  /* assign a new runid if it is later in time than the current value   */
	  sprintf(temprunid,"%s",runid2);
	  flag_match=1;
	}
      }
      /* closes up the 2nd while statement   */    
    }

    if(flag_match) {
      if(!strcmp(project,"DES")) {
	if(!strncmp(&(temprunid[strlen(temprunid)-1]),"_",1)) {
	  if(!flag_noparallel)
	    fprintf(filetemp,"%s %s %s  %s%s_%s/data/%s/%s/%s/%s_%s\n",
		    tempimagename,tempband,tempccd_number,temprunid,tempnite,tempccd_number,tempnite,tempband,tempimagename,tempimagename,tempccd_number); 
	  else
	    fprintf(filetemp,"%s %s %s  %s%s/data/%s/%s/%s/%s_%s\n",
		    tempimagename,tempband,tempccd_number,temprunid,tempnite,tempnite,tempband,tempimagename,tempimagename,tempccd_number); 
	}
	else {
	  if(!flag_noparallel)
	    fprintf(filetemp,"%s %s %s  %s_%s/data/%s/%s/%s/%s_%s\n",
		    tempimagename,tempband,tempccd_number,temprunid,tempnite,tempccd_number,tempnite,tempband,tempimagename,tempimagename,tempccd_number); 
	  else
	    fprintf(filetemp,"%s %s %s  %s_%s_%s/data/%s/%s/%s/%s_%s\n",
		    tempimagename,tempband,tempccd_number,temprunid,tempnite,tempnite,tempband,tempimagename,tempimagename,tempccd_number); 
	}
      }
      if(strcmp(project,"DES")) {
	if(!strncmp(&(temprunid[strlen(temprunid)-1]),"_",1))
	  fprintf(filetemp,"%s %s %s  %s%s/data/%s/%s/%s/%s_%s\n",
		  tempimagename,tempband,tempccd_number,temprunid,tempnite,tempnite,tempband,tempimagename,tempimagename,tempccd_number); 
	else
	  fprintf(filetemp,"%s %s %s  %s_%s/data/%s/%s/%s/%s_%s\n",
		  tempimagename,tempband,tempccd_number,temprunid,tempnite,tempnite,tempband,tempimagename,tempimagename,tempccd_number); 
      }
    }
    fclose(filein2);
    
    
    /* close 1st while loop   */
    loop++;
  }
  
  fclose(filein);
  fclose(filetemp);

  /* prepare file pointers */
  filemain=(FILE **)calloc(numlists,sizeof(FILE)); 
  if(numlists==1) {
    if(!strcmp(project,"DES")) 
      sprintf(filename,"%s_01",fileroot);
    else
      sprintf(filename,"%s",fileroot);
    filemain[0]=fopen(filename,"w");
  }
  else {
    for (i=0;i<numlists;i++) {
      sprintf(filename,"%s_%02d",fileroot,i+1);
      filemain[i]=fopen(filename,"w");
    }
  }
  
  if(flag_stdlist) {
    stdmain=(FILE **)calloc(numlists,sizeof(FILE)); 
    if(numlists==1) {
      if(!strcmp(project,"DES")) 
	sprintf(stdname,"%s_01",stdroot);
      else
	sprintf(stdname,"%s",stdroot);
      stdmain[0]=fopen(stdname,"w");
    }
    else {
      for (i=0;i<numlists;i++) {
	sprintf(stdname,"%s_%02d",stdroot,i+1);
	stdmain[i]=fopen(stdname,"w");
      }
    }
  }

  /* count the input list */
  Ntotal=0;
  infile=fopen("temp.dat","r");
  while(fscanf(infile,"%s %s %s %s",imagename, band, ccd_string, impath)!=EOF) {
    if(!Ntotal) sprintf(prev_imname,"%s",imagename);
    Ntotal++;
  }
  fclose(infile);

  threshold=Ntotal/numlists;
  if(!strcmp(project,"DES")) 
    if(threshold < DECAM) 
      threshold=DECAM;
  if(strcmp(project,"DES")) 
    if(threshold < MOSAIC) 
      threshold=MOSAIC;
  
  /* reopen for process */
  Nexposure=1; count=0; k=0;
  infile=fopen("temp.dat","r");
  while(fscanf(infile,"%s %s %s %s",imagename, band, ccd_string, impath)!=EOF) {
      
    count++;

    /* compare current imname with prev_imname */
    if (strcmp(imagename,prev_imname)) { /* if they are different */
      Nexposure++;
      sprintf(prev_imname,"%s",imagename);
      
      if(count > ((k+1)*threshold)) k++;
      if(k==(numlists)) k=numlists-1;

      
      fprintf(filemain[k],"%s %s %s  %s\n",imagename,band,ccd_string,impath);
      
      if(flag_stdlist)
	fprintf(stdmain[k],"%s_im.fits\n",impath);
    }
    else  { /* simply output the list to k-th file */
	fprintf(filemain[k],"%s %s %s  %s\n",imagename,band,ccd_string,impath);
	
	if(flag_stdlist)
	  fprintf(stdmain[k],"%s_im.fits\n",impath);
    }
  }
  fclose(infile);

  /* close the files */
  for (i=0;i<numlists;i++) fclose(filemain[i]);

  if(flag_stdlist) {
    for (i=0;i<numlists;i++) 
      fclose(stdmain[i]);
  }

  system("/bin/rm mkscampcatlist.sqlquery mkscampcatlist.sqlquery2 sql_list temp.dat");

  return (0);
}

substr(char dest[], char src[], int offset, int lenn)
{
  int i;
  for(i=0; i < lenn && src[offset + i] != '\0'; i++)
    dest[i] = src[i + offset];
  dest[i] = '\0';
}

#undef DECAM 
#undef MOSAIC 
