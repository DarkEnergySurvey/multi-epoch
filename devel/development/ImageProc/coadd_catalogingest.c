#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define VERSION 1.0

main(argc,argv)
     int argc;
     char *argv[];
{
  char catin_g[1000],catin_r[1000],catin_i[1000],catin_z[1000],
       binpath[1000],etcpath[1000],command[1000],tilename[100],
       arnode[100],arsites[100],dblogin[1000],
       runid[500],arroot[1000],line[1000],sqlcall[1000];
  int i,j,imageid_g,imageid_r,imageid_i,imageid_z;
  int flag_binpath=0,flag_etcpath=0;
  float equinox;
  FILE *pip,*fsqlout;
  void select_dblogin(),select_archivenode();

  if (argc<2) {
    printf("Usage: %s <tilename> <runid> <archive node> <equinox>\n",argv[0]);
    printf("    Option:\n");
    printf("          -binpath <binpath>\n");
    exit(0);
  }

  /* hardwire now; add checking inputs later */
  sprintf(tilename,"%s",argv[1]);
  sprintf(runid,"%s",argv[2]);
  sprintf(arnode,"%s",argv[3]);
  sscanf(argv[4],"%f",&equinox);

  /* process the command line */
  for (i=1;i<argc;i++) {
  
    if (!strcmp(argv[i],"-binpath"))  {
      flag_binpath=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -binpath option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
        sprintf(binpath,"%s",argv[i+1]);
        if (!strncmp(&binpath[0],"-",1)) {
          printf(" ** %s error: wrong input of <binpath>\n",argv[0]);
          exit(0);
        }
      }
    }    
  }
  
 /* grab dblogin */
  select_dblogin(dblogin);
  /* extract archive node information */
  select_archivenode(dblogin,arnode,arroot,arsites);

  /* hardwire the coadd catalog names (make flexible later */
  sprintf(catin_g,"%s/coadd/%s/%s/%s_g_cat.fits",arroot,runid,tilename,tilename);
  sprintf(catin_r,"%s/coadd/%s/%s/%s_r_cat.fits",arroot,runid,tilename,tilename);
  sprintf(catin_i,"%s/coadd/%s/%s/%s_i_cat.fits",arroot,runid,tilename,tilename);
  sprintf(catin_z,"%s/coadd/%s/%s/%s_z_cat.fits",arroot,runid,tilename,tilename);

  /* construct sql call to get tileinfo */
  sprintf(sqlcall, "${ORACLE_HOME}/bin/sqlplus -S %s < coadd.sql",dblogin);  

  fsqlout=fopen("coadd.sql", "w");
  fprintf(fsqlout,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
  fprintf(fsqlout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
  fprintf(fsqlout,"SELECT imageid ");
  fprintf(fsqlout,"FROM Files WHERE ");
  fprintf(fsqlout,"tilename=\'%s\' and band=\'g\' and imagetype=\'coadd\' and runiddesc=\'%s\';\n",tilename,runid);
  fprintf(fsqlout,"exit;\n");
  fclose(fsqlout);

  pip=popen(sqlcall, "r");
  fgets(line,1000,pip); 
  sscanf(line,"%d",&imageid_g); 
  pclose(pip);


  fsqlout=fopen("coadd.sql", "w");
  fprintf(fsqlout,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
  fprintf(fsqlout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
  fprintf(fsqlout,"SELECT imageid ");
  fprintf(fsqlout,"FROM Files WHERE ");
  fprintf(fsqlout,"tilename=\'%s\' and band=\'r\' and imagetype=\'coadd\' and runiddesc=\'%s\';\n",tilename,runid);
  fprintf(fsqlout,"exit;\n");
  fclose(fsqlout);

  pip=popen(sqlcall, "r");
  fgets(line,1000,pip); 
  sscanf(line,"%d",&imageid_r); 
  pclose(pip);

  fsqlout=fopen("coadd.sql", "w");
  fprintf(fsqlout,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
  fprintf(fsqlout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
  fprintf(fsqlout,"SELECT imageid ");
  fprintf(fsqlout,"FROM Files WHERE ");
  fprintf(fsqlout,"tilename=\'%s\' and band=\'i\' and imagetype=\'coadd\' and runiddesc=\'%s\';\n",tilename,runid);
  fprintf(fsqlout,"exit;\n");
  fclose(fsqlout);

  pip=popen(sqlcall, "r");
  fgets(line,1000,pip); 
  sscanf(line,"%d",&imageid_i); 
  pclose(pip);

  fsqlout=fopen("coadd.sql", "w");
  fprintf(fsqlout,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
  fprintf(fsqlout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
  fprintf(fsqlout,"SELECT imageid ");
  fprintf(fsqlout,"FROM Files WHERE ");
  fprintf(fsqlout,"tilename=\'%s\' and band=\'z\' and imagetype=\'coadd\' and runiddesc=\'%s\';\n",tilename,runid);
  fprintf(fsqlout,"exit;\n");
  fclose(fsqlout);

  pip=popen(sqlcall, "r");
  fgets(line,1000,pip); 
  sscanf(line,"%d",&imageid_z); 
  pclose(pip);

  system("rm coadd.sql");


  /* run catalog_ingest for g-band */
  if(flag_binpath)
    sprintf(command,"%s/catalog_ingest %s %d g %.2f > gband.out",binpath,catin_g,imageid_g,equinox);
  else
    sprintf(command,"catalog_ingest %s %d g %.2f > gband.out",catin_g,imageid_g,equinox);
  system(command);

  printf("%s\n",command);

  /* run catalog_ingest for r-band */
  if(flag_binpath)
    sprintf(command,"%s/catalog_ingest %s %d r %.2f > rband.out",binpath,catin_r,imageid_r,equinox);
  else
    sprintf(command,"catalog_ingest %s %d r %.2f > rband.out",catin_r,imageid_r,equinox);
  system(command);

  printf("%s\n",command);
  
  /* run catalog_ingest for i-band */
  if(flag_binpath)
    sprintf(command,"%s/catalog_ingest %s %d i %.2f > iband.out",binpath,catin_i,imageid_i,equinox);
  else
    sprintf(command,"catalog_ingest %s %d i %.2f > iband.out",catin_i,imageid_i,equinox);
  system(command);

  printf("%s\n",command);

  /* run catalog_ingest for z-band */
  if(flag_binpath)
    sprintf(command,"%s/catalog_ingest %s %d z %.2f > zband.out",binpath,catin_z,imageid_z,equinox);
  else
    sprintf(command,"catalog_ingest %s %d z %.2f > zband.out",catin_z,imageid_z,equinox);
  system(command);

  printf("%s\n",command);

  /* run awk to replace pipe */
  sprintf(command,"awk '{ gsub(/\\|/, \" \"); print }' gband.out > gband1.out");
  system(command);
 
  sprintf(command,"awk '{ gsub(/\\|/, \" \"); print }' rband.out > rband1.out");
  system(command);

  sprintf(command,"awk '{ gsub(/\\|/, \" \"); print }' iband.out > iband1.out");
  system(command);

  sprintf(command,"awk '{ gsub(/\\|/, \" \"); print }' zband.out > zband1.out");
  system(command);

  /* get zp from catalog */
  sprintf(command,"gawk '{printf \"\"$4\"\\n\"}' gband1.out > g_zp.out");
  system(command);

  sprintf(command,"gawk '{printf \"\"$4\"\\n\"}' rband1.out > r_zp.out");
  system(command);

  sprintf(command,"gawk '{printf \"\"$4\"\\n\"}' iband1.out > i_zp.out");
  system(command);

  sprintf(command,"gawk '{printf \"\"$4\"\\n\"}' zband1.out > z_zp.out");
  system(command);

  /* get mag from catalog */
  sprintf(command,"gawk '{printf \"\"$7\"\\t\"$8\"\\t\"$9\"\\t\"$10\"\\t\"$11\"\\t\"$12\"\\t\"$13\"\\t\"$14\"\\t\"$15\"\\t\"$16\"\\t\"$17\"\\t\"$18\"\\t\"$19\"\\t\"$20\"\\n\"}' gband1.out > g_mag.out");
  system(command);

  sprintf(command,"gawk '{printf \"\"$7\"\\t\"$8\"\\t\"$9\"\\t\"$10\"\\t\"$11\"\\t\"$12\"\\t\"$13\"\\t\"$14\"\\t\"$15\"\\t\"$16\"\\t\"$17\"\\t\"$18\"\\t\"$19\"\\t\"$20\"\\n\"}' rband1.out > r_mag.out");
  system(command);

  sprintf(command,"gawk '{printf \"\"$7\"\\t\"$8\"\\t\"$9\"\\t\"$10\"\\t\"$11\"\\t\"$12\"\\t\"$13\"\\t\"$14\"\\t\"$15\"\\t\"$16\"\\t\"$17\"\\t\"$18\"\\t\"$19\"\\t\"$20\"\\n\"}' iband1.out > i_mag.out");
  system(command);

  sprintf(command,"gawk '{printf \"\"$7\"\\t\"$8\"\\t\"$9\"\\t\"$10\"\\t\"$11\"\\t\"$12\"\\t\"$13\"\\t\"$14\"\\t\"$15\"\\t\"$16\"\\t\"$17\"\\t\"$18\"\\t\"$19\"\\t\"$20\"\\n\"}' zband1.out > z_mag.out");
  system(command);
 

  /* get object number from g */
  sprintf(command,"gawk '{printf \"\"$6\"\\n\"}' gband1.out > objnum.out");
  system(command);
 
  /* get alpha_j2000 to Y_image from g */
  sprintf(command,"gawk '{printf \"\"$21\"\\t\"$22\"\\t\"$25\"\\t\"$27\"\\t\"$29\"\\t\"$31\"\\t\"$32\"\\t\"$33\"\\n\"}' gband1.out > alpha2yimage.out");
  system(command);
 
  /* get theta_image to flags */
  sprintf(command,"gawk '{printf \"\"$48\"\\t\"$49\"\\t\"$50\"\\t\"$51\"\\t\"$52\"\\n\"}' gband1.out > g_theta2flag.out");
  system(command);

  sprintf(command,"gawk '{printf \"\"$48\"\\t\"$49\"\\t\"$50\"\\t\"$51\"\\t\"$52\"\\n\"}' rband1.out > r_theta2flag.out");
  system(command);

  sprintf(command,"gawk '{printf \"\"$48\"\\t\"$49\"\\t\"$50\"\\t\"$51\"\\t\"$52\"\\n\"}' iband1.out > i_theta2flag.out");
  system(command);

  sprintf(command,"gawk '{printf \"\"$48\"\\t\"$49\"\\t\"$50\"\\t\"$51\"\\t\"$52\"\\n\"}' zband1.out > z_theta2flag.out");
  system(command);
 
  /* get htmid etc from g */
  //sprintf(command,"gawk '{printf \"\"$53\" \"$54\" \"$55\" \"$56\" \\n\"}' gband1.out > htmidc.out");
  //system(command);
  
  /* for the zp err and zp id, set to 0 at the moment? */
  sprintf(command,"gawk '{printf \"\"0.0\"\\t\"0.0\"\\t\"0.0\"\\t\"0.0\"\\t\"0\"\\t\"0\"\\t\"0\"\\t\"0\"\\n\"}' gband1.out > zpinfo.out");
  system(command);
 
  /* for the equinox to imageid */
  sprintf(command,"gawk '{printf \"\"%2.2f\"\\t\"$53\"\\t\"$54\"\\t\"$55\"\\t\"$56\"\\t\"0\"\\t\"%d\"\\t\"%d\"\\t\"%d\"\\t\"%d\"\\n\"}' gband1.out > eq2imagid.out",equinox,imageid_g,imageid_r,imageid_i,imageid_z);
  system(command);

  /* paste the *out together */
  sprintf(command,"paste eq2imagid.out g_zp.out r_zp.out i_zp.out z_zp.out zpinfo.out objnum.out g_mag.out r_mag.out i_mag.out z_mag.out alpha2yimage.out g_theta2flag.out r_theta2flag.out i_theta2flag.out z_theta2flag.out > temp.out");
  system(command);

  /* put the pipe back */
  sprintf(command,"awk '{ gsub(/\\t/, \"|\"); print }' temp.out > %s_ingest.dat",tilename);
  system(command);

  /* clean up */
  system("rm *.out");
}

#undef VERSION

