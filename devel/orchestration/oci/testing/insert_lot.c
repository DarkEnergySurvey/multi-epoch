/* 
** run at this level:
**
**    runCatalog_ingest.c  -- wrapper program for running
**    catalog_ingest, a program that reads FITS catalogs and ingests them
**    into the Archive
**
** runid/data/nite/band/imagename/imagename_cat.fits
*/
#include "imageproc.h"
#include <unistd.h>
#include "desw_rel.h"
#include "insert_lot.h"

#define LINK "dber.ncsa.uiuc.edu/sony"
#define USER "sony_admin"
#define PASS "sonymgr"


desw_ctx ctx;
r_set rs;
sword retval = 0;


main(argc,argv)
int argc;
char *argv[];
{
  char imagename[800], command[15000], sqlldr_ctrl[800], imagenamesave[800],
      cleanup[20], sqlldr_data[800], sqlldr_root[800], nite[100], runid[100],
      band[100], filelist[MAXLISTS][800], line[1000], newimagename[800],
      tilename[100], imagetype[100], filename[800], sqlcall[800],
      inputimage[MAXIMAGE][800], binpath[800], runids[1000][100],
      templist[STRING_FIELDS], tmp[800];
  int k, j, i, len, imnum=0, flag_quiet=0, num_data, nrunids=0, nlists=0,
      flag_cat=0, flag_cleanup=0, flag_bin=0, flag_sqlldrctrl=0,
      flag_sqlldrdata=0, fileread, flag_sqlldr_multiingest=0, ingest_count=0,
      sqlldr_multiingest_limit=0, sleepcounter, imageid, ccdnum, flag, nfiles,
      match, num_tot=0, num_current=0, num_line, flag_ccd=0, select_ccdnum,
      numdbjobs();
  FILE *fin, *pip, *fin2, *out;
  void filename_resolve(), splitstring();
  struct imageinfo
  {
    int imageid, ccdnum;
    float equinox;
    char nite[100], band[100], runid[100], tilename[100], imagename[100],
        imagetype[100];
  }*im;

  if (argc < 2)
  {
    printf("Usage: %s <list1> <list2> ... <listN>\n", argv [0]);
    printf("  Options:\n");
    printf("  -bin <dir>\n");
    //         printf("  -sqlldr_ctrl <file>\n");
    //          printf("  -sqlldr_data <file>\n");
    //          printf("  -sqlldr_multiingest <#>\n");
    printf("  -ccd <#>\n");
    printf("  -quiet\n");
    //          printf("  <#_cat.fits> <imageid (from Files table)> <band> <equinox>\n");
    //          printf("  -ascii (if the catalog is in ascii) \n");
    exit(0);
  }

  /* set up defaults */
  binpath[0]=0;
  sprintf(sqlldr_data, "runCatalog_ingest.data");

  /* ************************************************************* */
  /* *************  process the command line ********************* */
  /* ************************************************************* */
  for (i=1; i<argc; i++)
  {
    flag=0;
    if (!strcmp(argv[i], "-quiet"))
      flag=flag_quiet=1;
    if (!strcmp(argv[i], "-sqlldr_data"))
    {
      flag=flag_sqlldrdata=1;
      i++;
      /* read bin */
      sscanf(argv[i], "%s", sqlldr_root);
      sprintf(sqlldr_data, "%s.sqldata", sqlldr_root);
    }
    if (!strcmp(argv[i], "-sqlldr_ctrl"))
    {
      flag=flag_sqlldrctrl=1;
      i++;
      /* read bin */
      sscanf(argv[i], "%s", sqlldr_ctrl);
    }
    if (!strcmp(argv[i], "-sqlldr_multiingest"))
    {
      flag=flag_sqlldr_multiingest=1;
      i++;
      /* read bin */
      sscanf(argv[i], "%d", &sqlldr_multiingest_limit);
    }
    if (!strcmp(argv[i], "-bin"))
    {
      flag=flag_bin=1;
      i++;
      /* read bin */
      sscanf(argv[i], "%s", binpath);
    }
    if (!strcmp(argv[i], "-ccd"))
    {
      flag=flag_ccd=1;
      i++;
      /* read bin */
      sscanf(argv[i], "%d", &select_ccdnum);
    }
    if (!flag)
    { /* assume it is a file list */
      sprintf(filelist[nlists], "%s", argv[i]);
      nlists++;
      if (nlists>=MAXLISTS)
      {
        printf("  **runCatalog_ingest:  MAXLISTS exceeded!\n");
        exit(0);
      }
    }
  }

for (j=0; j<nlists; j++)
{
    printf( "FILE NAME: %s\n", filelist[j]);
}
printf( "+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*\n\n\n" );


  //Connect to DB
  initialize(&ctx, (text *)USER, (text *)PASS, (text *)LINK);
  cleanup_db( &ctx );

  /* ************************************************************* */
  /* now cycle through lists to confirm cat files and get runids * */
  /* ************************************************************* */
  for (j=0; j<nlists; j++)
  {
    fin=fopen(filelist[j], "r");
    /* check list file to confirm it exists */
    if (fin == NULL)
    {
      printf("  ** runCatalog_ingest:  List file \"%s\" not found.\n",
             filelist[j]);
      exit(0);
    }
    /* confirm that it contains FITS catalog files */
    while (fscanf(fin, "%s", imagename)!=EOF)
    {
      /* resolve filename into file information */



      filename_resolve(imagename, runid, nite, band, tilename, imagetype,
                       newimagename, &ccdnum);
      if (!flag_quiet)
        printf("%s\n runid=%s nite=%s band=%s tilename=%s imagetype=%s ccdnum=%d newimagename=%s\n",
               imagename, runid, nite, band, tilename, imagetype, ccdnum,
               newimagename);
      /* confirm it is a catalog file */
      if (strncmp(imagetype, "catalog", 7))
      {
        printf("  **runCatalog_ingest: File must contain list of FITS catalogs **\n");
        exit(0);
      }
      /* add runid to current list */
      /* but only if correct ccdnum if select_ccdnum filter set */
      if (!flag_ccd || (flag_ccd && select_ccdnum==ccdnum))
      {
        flag=0;
        for (i=0; i<nrunids; i++)
          if (!strcmp(runid, runids[i]))
            flag=1;
        if (!flag)
        {
          sprintf(runids[nrunids], "%s", runid);
          nrunids++;
        }
      }
    }
    fclose(fin);
  }




  if (!flag_quiet)
  {
    printf("  Catlist contains %d runid", nrunids);
    if (nrunids>1)
      printf("s\n");
    else
      printf("\n");
  }

exit(1);

  /* ************************************************************* */
  /* ****** now query for list of images with these runids ******* */
  /* ************************************************************* */

  char ids[5000];
  
  for( i =0; i < nrunids; i++ )
  {
    sprintf( ids, "%s %s runiddesc = '%s' ", ids, ((i > 0) ? "OR" : ""), runids[i] );
  }
  
  char stmt[5000];
  sprintf( stmt, "SELECT COUNT(*) FROM files WHERE %s", ids );
  for( i = 1; i < nrunids; i++ )
  {
    sprintf( stmt, "OR runiddesc = '%s' ", runids[i]);
  }
  
  sql_select_execute(&ctx, (text *)stmt, &rs);

  while ( (retval = fetch( &ctx, &rs)) == OCI_SUCCESS || 
          (retval == OCI_SUCCESS_WITH_INFO))
  {
    for (k = 0; k < rs.parmcnt; k++)
    {
      if (strlen((void *)rs.value[k]) != 0)
      {
        //        printf( "CTX2(%d): ", k );
        sscanf( rs.value[k], "%d", nfiles );
        printf("%-*s ", rs.pcoll[k], (void *)rs.value[k],
               strlen((void *)rs.value[k]));
      }
    }
    printf("\n");
  }
  
  

  sprintf(stmt, "SELECT imageid, nite, band, tilename, runiddesc, imagename, imagetype, ccd_number, equinox FROM files WHERE %s ", ids);
  for (i = 1; i < nrunids; i++)
  {
    sprintf(stmt, "OR runiddesc = '%s' ", runids[i]);
  }

  sql_select_execute(&ctx, (text *)stmt, &rs);

  while ( (retval = fetch( &ctx, &rs)) == OCI_SUCCESS || (retval
      == OCI_SUCCESS_WITH_INFO))
  {

        sscanf(rs.value[0], "%d", &(im[i].imageid));
        sscanf(rs.value[1], "%s", im[i].nite);
        sscanf(rs.value[2], "%s", im[i].band);
        sscanf(rs.value[3], "%s", im[i].tilename);
        sscanf(rs.value[4], "%s", im[i].runid);
        sscanf(rs.value[5], "%s", im[i].imagename);
        sscanf(rs.value[6], "%s", im[i].imagetype);
        sscanf(rs.value[7], "%d", &(im[i].ccdnum));
        sscanf(rs.value[8], "%f", &(im[i].equinox));
        if (!flag_quiet)
          printf("  imageid: %d nite: %s band: %s tilename: %s runid: %s imagename: %s imagetype: %s ccdnum: %d\n",
                 im[i].imageid, im[i].nite, im[i].band, im[i].tilename,
                 im[i].runid, im[i].imagename, im[i].imagetype, im[i].ccdnum);        
  }
    
    
    
    
    
    
  
  


  /* ************************************************************* */
  /* now cycle through lists to catalog_ingest the files contained */
  /* ************************************************************* */
  num_tot=num_current=0;
  for (j=0; j<nlists; j++)
  {
    fin=fopen(filelist[j], "r");
    if (!flag_quiet)
      printf(" ** Processing list %s\n", filelist[j]);
    /* read through image list */
    do
    {
      fileread=fscanf(fin, "%s", imagename);
      
      
      
      
      
      
      
      /* prepare output data file */
      if (fileread==EOF || num_current==0 || num_current>SQLLDR_LIMIT)
      {
        if (num_current && flag_sqlldrctrl)
        { /* ingest current data */
          if (flag_sqlldr_multiingest)
          {
            /* make sure we don't exceed multiingest limits*/
            printf("\n");
            sleepcounter=0;
            do
            {
              if (sleepcounter>0)
                sleep(10);
              sleepcounter++;
              printf(".");
              fflush(stdout);
            } while (numdbjobs("sqlldr", sqlldr_root)>=sqlldr_multiingest_limit);
            printf("\n");
            sprintf(command,
                    "${ORACLE_HOME}/bin/sqlldr pipeline/dc01user@charon.ncsa.uiuc.edu/des control=\"%s\" data=\"%s_%02d\" log=\"%s.sqllog_%02d\" bad=\"%s.sqlbad_%02d\" rows=5000 bindsize=20000000 readsize=20000000 &",
                    sqlldr_ctrl, sqlldr_data, ingest_count, sqlldr_root,
                    ingest_count, sqlldr_root, ingest_count);
          }
          else
            sprintf(command,
                    "${ORACLE_HOME}/bin/sqlldr pipeline/dc01user@charon.ncsa.uiuc.edu/des control=\"%s\" data=\"%s_%02d\" log=\"%s.sqllog_%02d\" bad=\"%s.sqlbad_%02d\" rows=5000 bindsize=20000000 readsize=20000000",
                    sqlldr_ctrl, sqlldr_data, ingest_count, sqlldr_root,
                    ingest_count, sqlldr_root, ingest_count);
          if (!flag_quiet)
          {
            printf("  %s\n", command);
            printf("  Ingesting %d objects\n\n", num_current);
            fflush(stdout);
          }
          system(command);
        }
        /* now open new data file */
        /*if (fopen(sqlldr_data,"w")!=NULL) {
         sprintf(command,"rm %s",sqlldr_data);
         system(command);
         }*/
        ingest_count++; /* increment file ingest counter */
        /* reset counter */
        num_tot+=num_current;
        num_current=0;
      }

      
      
      
      
      
      
      /* resolve catalog name */
      if (fileread!=EOF)
        filename_resolve(imagename, runid, nite, band, tilename, imagetype,
                         newimagename, &ccdnum);

      if (fileread!=EOF && (!flag_ccd || (flag_ccd && ccdnum==select_ccdnum)))
      {
        if (!flag_quiet)
          printf("  Catalog %s: ", imagename);
        //if (!flag_quiet) printf("  nite: %s band: %s tilename: %s runid: %s imagename: %s imagetype: %s ccdnum: %d\n",
        // nite,band,tilename,runid,newimagename,imagetype,ccdnum);
        /* find the corresponding image file and imageid */
        match=-1;
        for (i=0; i<nfiles; i++)
        {
          if (!strcmp(newimagename, im[i].imagename) && !strcmp(runid,
                                                                im[i].runid)
              && !strcmp(nite, im[i].nite) && !strcmp(tilename, im[i].tilename)
              && !strcmp(band, im[i].band) && ccdnum==im[i].ccdnum
              && strcmp(im[i].imagetype, "catalog"))
          {
            if (match<0)
              match=i;
            else
            {
              printf("  **runCatalog_ingest:  more than one image match found for catalog %s:  %d (imageid=%d) and %d (imageid=%d)\n",
                     imagename, match, im[match].imageid, i, im[i].imageid);
              exit(0);
            }
          }
        }
        if (match<0)
        {
          printf("\n  **runCatalog_ingest:  no matching image file found for %s\n",
                 imagename);
          exit(0);
        }

        command[0]=0; /* clear the last command */
        

        insert_lots( imagename, im[match].imageid, band, im[match].equinox );
/******************************************        
        if (flag_bin)
          sprintf(command, "%s/catalog_ingest %s %d %s %.2f >> %s_%02d",
                  binpath, imagename, im[match].imageid, band,
                  im[match].equinox, sqlldr_data, ingest_count);
        else
          sprintf(command, "catalog_ingest %s %d %s %.2f >> %s_%02d",
                  imagename, im[match].imageid, band, im[match].equinox,
                  sqlldr_data, ingest_count);
********************************************/                  
        /* report and execute the command */
        if (!flag_quiet)
        {
          printf("%s\n", command);
          fflush(stdout);
        }
        system(command);
        /* count the number of entries in the data file */
        sprintf(command, "wc -l %s_%02d", sqlldr_data, ingest_count);
        pip=popen(command, "r");
        fscanf(pip, "%d %s", &num_line, tmp);
        pclose(pip);
        num_current=num_line;
        if (!flag_quiet)
          printf(" %d objects extracted\n", num_line);
      }
    } while (fileread!=EOF);/* completed processing of single catalog */
    fclose(fin);
  } /* completed processing of a single list */
  if (!flag_quiet)
  {
    printf("  runCatalog_ingest complete:  %d objects ingested in %d batches\n",
           num_tot, ingest_count-1);
  }

  return (0);
}

/* subroutine that returns the number of active DB jobs with program like 'name' */
int numdbjobs(name,root)
char name[],root[];
{
  FILE *out, *pip;
  int numjobs;
  char queryfile[500], query[500];

  sprintf(queryfile, "%s.numdbjobs.sql", root);
  out=fopen(queryfile, "w");
  fprintf(out, "SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP '|'");
  fprintf(out, "HEAD OFF TRIMS ON LINESIZE 1000;\n");
  /* first find out how many solutions to expect */
  fprintf(out, "SELECT count(DB_system_id)\n");
  fprintf(out, "FROM DES_ACTIVE_SESSIONs\n");
  fprintf(out, "WHERE program like '\%%");
  fprintf(out, "%s", name);
  fprintf(out, "\%';\n");
  fprintf(out, "exit;\n");
  fclose(out);

  sprintf(query,
          "${ORACLE_HOME}/bin/sqlplus -S pipeline/dc01user@charon.ncsa.uiuc.edu/des < %s",
          queryfile);
  pip=popen(query, "r");
  fscanf(pip, "%d", &numjobs);
  pclose(pip);
  return (numjobs);
}




struct imageinfo
{
  int imageid, ccdnum;
  float equinox;
  char nite[100], band[100], runid[100], tilename[100], imagename[100],
      imagetype[100];
}*im;



insert_lots( imagename, imageid, band, equinox )
char *imagename;
int   imageid;
char *band;
float equinox;
{
char instStmt[5000];

/*********************
  if (argc < 2)
  {
    printf(
           "Usage: %s <#_cat.fits> <imageid (from Files table)> <band> <equinox>\n",
           argv[0]);
    printf("          -ascii (if the catalog is in ascii) \n", argv[0]);
    exit(0);
  }

  imageid = atoi(argv[2]);
  sprintf(band, "%s", argv[3]);
  sscanf(argv[4], "%f", &equinox);

  for (i=4; i<argc; i++)
  {
    if (!strcmp(argv[i], "-ascii"))
      flag_ascii=1;
  }
**************************/

  //cleanup(&ctx);

  if (!flag_ascii)
  {
    if (!fits_open_file(&fptr, imagename, READONLY, &status))
    {
      fits_read_key_flt(fptr, "SEXMGZPT", &magzp, comment, &status);

      fits_get_hdu_num(fptr, &hdunum);

      if (hdunum == 1)
      {
        /* This is the primary array;  try to move to the */
        /* first extension and see if it is a table */
        fits_movabs_hdu(fptr, 2, &hdutype, &status);
      }
      else
        fits_get_hdu_type(fptr, &hdutype, &status); /* Get the HDU type */

      if (hdutype == IMAGE_HDU)
        printf("Error: this program only displays tables, not images\n");
      else
      {
        fits_get_num_rows(fptr, &nrows, &status);
        fits_get_num_cols(fptr, &ncols, &status);
        /* print each column, row by row (there are faster ways to do this) */
        val = value;
      }

      /* new lines from Joe's code */

      repeat=(long *)calloc(ncols, sizeof(long));
      width=(long *)calloc(ncols, sizeof(long));
      typecode=(int *)calloc(ncols, sizeof(int));
      pointer=(int *)calloc(ncols, sizeof(int *));
      nullpointer=(int *)calloc(ncols, sizeof(int *));
      prstring=(char **)calloc(ncols, sizeof(char *));

      for (i=0; i<ncols; i++)
      {
        if (fits_get_coltype(fptr, i+1, &(typecode[i]), &(repeat[i]),
                             &(width[i]), &status))
          ;
        nullpointer[i]=(float *)&floatnull;
      }

      nullpointer[5]=nullpointer[6]=(double *)&doublenull;
      nullpointer[7]=nullpointer[8]=(double *)&doublenull;
      nullpointer[9]=nullpointer[10]=(double *)&doublenull;
      nullpointer[11]=nullpointer[12]=(double *)&doublenull;
      nullpointer[13]=nullpointer[14]=(double *)&doublenull;
      nullpointer[22]=nullpointer[23]=(double *)&doublenull;
      nullpointer[24]=nullpointer[25]=(double *)&doublenull;
      nullpointer[26]=nullpointer[27]=(double *)&doublenull;

      nullpointer[0]=(int *)&intnull;
      nullpointer[18]=(int *)&intnull;
      nullpointer[19]=(int *)&intnull;
      nullpointer[20]=(int *)&intnull;
      nullpointer[21]=(int *)&intnull;
      nullpointer[36]=(short *)&shortnull;

      /* now prepare the data vectors for each column; multi-dimensional first */

      magaper=(float *)calloc(nrows*6, sizeof(float));
      pointer[3]=(float *)magaper;

      magerrap=(float *)calloc(nrows*6, sizeof(float));
      pointer[4]=(float *)magerrap;

      /* prepare data vectors for single entry */
      objnum=(int *)calloc(nrows, sizeof(int));
      pointer[0]=(int *)objnum;

      magauto=(float *)calloc(nrows, sizeof(float));
      pointer[1]=(float *)magauto;

      magerrauto=(float *)calloc(nrows, sizeof(float));
      pointer[2]=(float *)magerrauto;

      alphara=(double *)calloc(nrows, sizeof(double));
      pointer[5]=(double *)alphara;

      deltadec=(double *)calloc(nrows, sizeof(double));
      pointer[6]=(double *)deltadec;

      alphapeak=(double *)calloc(nrows, sizeof(double));
      pointer[7]=(double *)alphapeak;

      deltapeak=(double *)calloc(nrows, sizeof(double));
      pointer[8]=(double *)deltapeak;

      x2world=(double *)calloc(nrows, sizeof(double));
      pointer[9]=(double *)x2world;

      x2errworld=(double *)calloc(nrows, sizeof(double));
      pointer[10]=(double *)x2errworld;

      y2world=(double *)calloc(nrows, sizeof(double));
      pointer[11]=(double *)y2world;

      y2errworld=(double *)calloc(nrows, sizeof(double));
      pointer[12]=(double *)y2errworld;

      xyworld=(double *)calloc(nrows, sizeof(double));
      pointer[13]=(double *)xyworld;

      xyerrworld=(double *)calloc(nrows, sizeof(double));
      pointer[14]=(double *)xyerrworld;

      threshold=(float *)calloc(nrows, sizeof(float));
      pointer[15]=(float *)threshold;

      ximage=(float *)calloc(nrows, sizeof(float));
      pointer[16]=(float *)ximage;

      yimage=(float *)calloc(nrows, sizeof(float));
      pointer[17]=(float *)yimage;

      xminimage=(int *)calloc(nrows, sizeof(int));
      pointer[18]=(int *)xminimage;

      yminimage=(int *)calloc(nrows, sizeof(int));
      pointer[19]=(int *)yminimage;

      xmaximage=(int *)calloc(nrows, sizeof(int));
      pointer[20]=(int *)xmaximage;

      ymaximage=(int *)calloc(nrows, sizeof(int));
      pointer[21]=(int *)ymaximage;

      x2image=(double *)calloc(nrows, sizeof(double));
      pointer[22]=(double *)x2image;

      x2errimage=(double *)calloc(nrows, sizeof(double));
      pointer[23]=(double *)x2errimage;

      y2image=(double *)calloc(nrows, sizeof(double));
      pointer[24]=(double *)y2image;

      y2errimage=(double *)calloc(nrows, sizeof(double));
      pointer[25]=(double *)y2errimage;

      xyimage=(double *)calloc(nrows, sizeof(double));
      pointer[26]=(double *)xyimage;

      xyerrimage=(double *)calloc(nrows, sizeof(double));
      pointer[27]=(double *)xyerrimage;

      aimage=(float *)calloc(nrows, sizeof(float));
      pointer[28]=(float *)aimage;

      aerrimage=(float *)calloc(nrows, sizeof(float));
      pointer[29]=(float *)aerrimage;

      bimage=(float *)calloc(nrows, sizeof(float));
      pointer[30]=(float *)bimage;

      berrimage=(float *)calloc(nrows, sizeof(float));
      pointer[31]=(float *)berrimage;

      thetaimage=(float *)calloc(nrows, sizeof(float));
      pointer[32]=(float *)thetaimage;

      thetaerrimage=(float *)calloc(nrows, sizeof(float));
      pointer[33]=(float *)thetaerrimage;

      ellipticity=(float *)calloc(nrows, sizeof(float));
      pointer[34]=(float *)ellipticity;

      class=(float *)calloc(nrows, sizeof(float));
      pointer[35]=(float *)class;

      fflags=(short *)calloc(nrows, sizeof(short));
      pointer[36]=(short *)fflags;

      htmID=(uint64 *)calloc(nrows, sizeof(uint64));

      cx=(double *)calloc(nrows, sizeof(double));

      cy=(double *)calloc(nrows, sizeof(double));

      cz=(double *)calloc(nrows, sizeof(double));

      /* this looks good to me */
      for (i=0; i<ncols; i++)
      {
        if (fits_read_col(fptr, typecode[i], i+1, 1, 1, nrows*repeat[i],
                          nullpointer[i], pointer[i], &anynul, &status))
          ;
      }
    }

    fits_close_file(fptr, &status);

    /* Output the column values */
    printf("ROW COUTN: %d\n", nrows-1);

    for (j=0; j<nrows; j++)
    {
      if (magerrauto[j] > 999.99)
        magerrauto[j] = 999.99;

      htmID[j] = cc_radec2ID(alphara[j], deltadec[j], htmdepth);

      getxyz(alphara[j], deltadec[j], &cx[j], &cy[j], &cz[j]);
    }

    if (status)
      fits_report_error(stderr, status); /* print any error message */

    //printf( "nrows: %d\n", nrows );
    insert_objects(&ctx);
    cleanup_db(&ctx);
    exit;

    //return(status);
  }
  else
  {

    if (!fits_open_file(&fptr, imagename, READONLY, &status))
      fits_read_key_flt(fptr, "SEXMGZPT", &magzp, comment, &status);
    fits_close_file(fptr, &status);

    sprintf(command, "wc -l %s", imagename);

    pip=popen(command, "r");
    fgets(line, 1000, pip);
    sscanf(line, "%d %s", &nrows, command);
    pclose(pip);

    fin=fopen(imagename, "r");

    for (j=0; j<nrows; j++)
    {
      fscanf(fin, "%d %lg %lg ", &objnum_ascii, &magauto_ascii,
             &magerrauto_ascii);
      fscanf(fin, "%lg %lg ", &magaper1_ascii, &magaper2_ascii);
      fscanf(fin, "%lg %lg ", &magaper3_ascii, &magaper4_ascii);
      fscanf(fin, "%lg %lg ", &magaper5_ascii, &magaper6_ascii);
      fscanf(fin, "%lg %lg ", &magerrap1_ascii, &magerrap2_ascii);
      fscanf(fin, "%lg %lg ", &magerrap3_ascii, &magerrap4_ascii);
      fscanf(fin, "%lg %lg ", &magerrap5_ascii, &magerrap6_ascii);

      fscanf(fin, "%lg %lg %lg %lg ", &alphara_ascii, &deltadec_ascii,
             &alphapeak_ascii, &deltapeak_ascii);
      fscanf(fin, "%lg %lg %lg %lg %lg %lg ", &x2world_ascii,
             &x2errworld_ascii, &y2world_ascii, &y2errworld_ascii,
             &xyworld_ascii, &xyerrworld_ascii);
      fscanf(fin, "%lg %lg %lg ", &threshold_ascii, &ximage_ascii,
             &yimage_ascii);
      fscanf(fin, "%d %d %d %d ", &xminimage_ascii, &yminimage_ascii,
             &xmaximage_ascii, &ymaximage_ascii);
      fscanf(fin, "%lg %lg %lg %lg ", &x2image_ascii, &x2errimage_ascii,
             &y2image_ascii, &y2errimage_ascii);
      fscanf(fin, "%lg %lg ", &xyimage_ascii, &xyerrimage_ascii);
      fscanf(fin, "%lg %lg %lg %lg ", &aimage_ascii, &aerrimage_ascii,
             &bimage_ascii, &berrimage_ascii);
      fscanf(fin, "%lg %lg %lg ", &thetaimage_ascii, &thetaerrimage_ascii,
             &ellipticity_ascii);
      fscanf(fin, "%lg %d", &class_ascii, &fflags_ascii);

      if (magerrauto_ascii > 999.99)
        magerrauto_ascii = 999.99;
      printf("%2.1f|%s|%2d|%2.1f|0.0|%10d|%8.4f|%8.4f|", equinox, band,
             imageid, magzp, objnum_ascii, magauto_ascii, magerrauto_ascii);
      if (magaper1_ascii > 999.99)
        magaper1_ascii = 999.99;
      printf("%8.4f|%8.4f|", magaper1_ascii, magerrap1_ascii);
      if (magaper2_ascii > 999.99)
        magaper2_ascii = 999.99;
      printf("%8.4f|%8.4f|", magaper2_ascii, magerrap2_ascii);
      if (magaper3_ascii > 999.99)
        magaper3_ascii = 999.99;
      printf("%8.4f|%8.4f|", magaper3_ascii, magerrap3_ascii);
      if (magaper4_ascii > 999.99)
        magaper4_ascii = 999.99;
      printf("%8.4f|%8.4f|", magaper4_ascii, magerrap4_ascii);
      if (magaper5_ascii > 999.99)
        magaper5_ascii = 999.99;
      printf("%8.4f|%8.4f|", magaper5_ascii, magerrap5_ascii);
      if (magaper6_ascii > 999.99)
        magaper6_ascii = 999.99;
      printf("%8.4f|%8.4f|", magaper6_ascii, magerrap6_ascii);

      printf("%11.7f|%11.7f|%11.7f|%11.7f|", alphara_ascii, deltadec_ascii,
             alphapeak_ascii, deltapeak_ascii);
      printf("%19.13e|%19.13e|%19.13e|%19.13e|%19.13e|", x2world_ascii,
             x2errworld_ascii, y2world_ascii, y2errworld_ascii, xyworld_ascii);
      printf("%19.13e|%12g|%10.3f|%10.3f|", xyerrworld_ascii, threshold_ascii,
             ximage_ascii, yimage_ascii);
      printf("%10d|%10d|%10d|%10d|", xminimage_ascii, yminimage_ascii,
             xmaximage_ascii, ymaximage_ascii);
      //if(y2image > 1e7)
      //y2image = 0.0;
      printf("%19.13e|%19.13e|%19.13e|%19.13e|", x2image_ascii,
             x2errimage_ascii, y2image_ascii, y2errimage_ascii);
      printf("%19.13e|%19.13e|%9.3f|%8.4f|", xyimage_ascii, xyerrimage_ascii,
             aimage_ascii, aerrimage_ascii);
      printf("%9.3f|%8.4f|%5.1f|%5.1f|", bimage_ascii, berrimage_ascii,
             thetaimage_ascii, thetaerrimage_ascii);
      printf("%8.3f|%5.2f|%d|", ellipticity_ascii, class_ascii, fflags_ascii);

      /* get htmID */
      htmID = cc_radec2ID(alphara_ascii, deltadec_ascii, htmdepth);
      printf("%Ld|", htmID);
      /* get cx,cy,cz */
      getxyz(alphara_ascii, deltadec_ascii, &cx, &cy, &cz);
      printf("%4.6f|%4.6f|%4.6f\n", cx, cy, cz);
    }
    fclose(fin);
  }

  return (0);
}

/*perform piecewise insert with polling*/
void insert_objects(ctxptr)
desw_ctx *ctxptr;
{
  text
      *ins_stmt1 =
          (text *)"INSERT INTO objects (OBJECT_ID, EQUINOX, BAND, HTMID, CX, CY, CZ, PARENTID, SOFTID, IMAGEID, ZEROPOINT, ERRZEROPOINT, ZEROPOINTID, OBJECT_NUMBER, MAG_AUTO, MAGERR_AUTO, MAG_APER_1, MAGERR_APER_1, MAG_APER_2, MAGERR_APER_2, MAG_APER_3, MAGERR_APER_3, MAG_APER_4, MAGERR_APER_4, MAG_APER_5, MAGERR_APER_5, MAG_APER_6, MAGERR_APER_6, ALPHA_J2000, DELTA_J2000, ALPHAPEAK_J2000, DELTAPEAK_J2000, X2_WORLD, ERRX2_WORLD, Y2_WORLD, ERRY2_WORLD, XY_WORLD, ERRXY_WORLD, THRESHOLD, X_IMAGE, Y_IMAGE, XMIN_IMAGE, YMIN_IMAGE, XMAX_IMAGE, YMAX_IMAGE, X2_IMAGE, ERRX2_IMAGE, Y2_IMAGE, ERRY2_IMAGE, XY_IMAGE, ERRXY_IMAGE, A_IMAGE, ERRA_IMAGE, B_IMAGE, ERRB_IMAGE, THETA_IMAGE, ERRTHETA_IMAGE, ELLIPTICITY, CLASS_STAR, FLAGS) VALUES (objects_seq.nextval, %2.1f, %s, :1, :2, :3, :4, :8, :9, %2d, %2.1f,NULL,NULL, :14, :15, :16, :17, :18, :19, :20, :21, :22, :23, :24, :25, :26, :27, :28, :29, :30, :31, :32, :33, :34, :35, :36, :37, :38, :39, :40, :41, :42, :43, :44, :45, :46, :47, :48, :49, :50, :51, :52, :53, :54, :55, :56, :57, :58, :59, :60)";

  sword status, i = 0;

  ub4 rlsk = 0;
  ub4 rcsk = 0;
  ub4 indsk = 0;

  char instStmt[5000];

  sprintf(
          instStmt,
          "INSERT INTO objects (OBJECT_ID, EQUINOX, BAND, HTMID, CX, CY, CZ, PARENTID, SOFTID, IMAGEID, ZEROPOINT, ERRZEROPOINT, ZEROPOINTID, OBJECT_NUMBER, MAG_AUTO, MAGERR_AUTO, MAG_APER_1, MAGERR_APER_1, MAG_APER_2, MAGERR_APER_2, MAG_APER_3, MAGERR_APER_3, MAG_APER_4, MAGERR_APER_4, MAG_APER_5, MAGERR_APER_5, MAG_APER_6, MAGERR_APER_6, ALPHA_J2000, DELTA_J2000, ALPHAPEAK_J2000, DELTAPEAK_J2000, X2_WORLD, ERRX2_WORLD, Y2_WORLD, ERRY2_WORLD, XY_WORLD, ERRXY_WORLD, THRESHOLD, X_IMAGE, Y_IMAGE, XMIN_IMAGE, YMIN_IMAGE, XMAX_IMAGE, YMAX_IMAGE, X2_IMAGE, ERRX2_IMAGE, Y2_IMAGE, ERRY2_IMAGE, XY_IMAGE, ERRXY_IMAGE, A_IMAGE, ERRA_IMAGE, B_IMAGE, ERRB_IMAGE, THETA_IMAGE, ERRTHETA_IMAGE, ELLIPTICITY, CLASS_STAR, FLAGS) VALUES (objects_seq.nextval, %2.1f, '%s', :1, :2, :3, :4, 0, 0, %2d, %2.1f,NULL,NULL, :14, :15, :16, :17, :18, :19, :20, :21, :22, :23, :24, :25, :26, :27, :28, :29, :30, :31, :32, :33, :34, :35, :36, :37, :38, :39, :40, :41, :42, :43, :44, :45, :46, :47, :48, :49, :50, :51, :52, :53, :54, :55, :56, :57, :58, :59, :60)",
          equinox, band, imageid, magzp);

  checkerr(ctxptr->errhp, OCIStmtPrepare(ctxptr->stmthp, ctxptr->errhp,

  (text *)instStmt, (ub4) strlen((char *)instStmt),

  (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 1,
                                       (dvoid *) &htmID[0],
                                        (sb4) sizeof(uint64), SQLT_INT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(uint64), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 2,
                                       (dvoid *) &cx[0], (sb4) sizeof(double),
                                       SQLT_FLT, (dvoid *) 0, (ub2 *)0,
                                       (ub2 *)0, (ub4) 0, (ub4 *) 0,
                                       (ub4) OCI_DEFAULT), __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 3,
                                       (dvoid *) &cy[0], (sb4) sizeof(double),
                                       SQLT_FLT, (dvoid *) 0, (ub2 *)0,
                                       (ub2 *)0, (ub4) 0, (ub4 *) 0,
                                       (ub4) OCI_DEFAULT), __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 4,
                                       (dvoid *) &cz[0], (sb4) sizeof(double),
                                       SQLT_FLT, (dvoid *) 0, (ub2 *)0,
                                       (ub2 *)0, (ub4) 0, (ub4 *) 0,
                                       (ub4) OCI_DEFAULT), __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 5,
                                       (dvoid *) &objnum[0], (sb4) sizeof(int),
                                       SQLT_INT, (dvoid *) 0, (ub2 *)0,
                                       (ub2 *)0, (ub4) 0, (ub4 *) 0,
                                       (ub4) OCI_DEFAULT), __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(int), indsk, rlsk,
                                               rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 6,
                                       (dvoid *) &magauto[0],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 7,
                                       (dvoid *) &magerrauto[0],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 8,
                                       (dvoid *) &magaper[0],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float)*6, indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 9,
                                       (dvoid *) &magerrap[0],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float)*6, indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 10,
                                       (dvoid *) &magaper[1],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float)*6, indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 11,
                                       (dvoid *) &magerrap[1],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float)*6, indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 12,
                                       (dvoid *) &magaper[2],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float)*6, indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 13,
                                       (dvoid *) &magerrap[2],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float)*6, indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 14,
                                       (dvoid *) &magaper[3],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float)*6, indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 15,
                                       (dvoid *) &magerrap[3],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float)*6, indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 16,
                                       (dvoid *) &magaper[4],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float)*6, indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 17,
                                       (dvoid *) &magerrap[4],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float)*6, indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 18,
                                       (dvoid *) &magaper[5],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float)*6, indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 19,
                                       (dvoid *) &magerrap[5],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float)*6, indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 20,
                                       (dvoid *) &alphara[0],
                                        (sb4) sizeof(double), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 21,
                                       (dvoid *) &deltadec[0],
                                        (sb4) sizeof(double), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 22,
                                       (dvoid *) &alphapeak[0],
                                        (sb4) sizeof(double), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 23,
                                       (dvoid *) &deltapeak[0],
                                        (sb4) sizeof(double), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 24,
                                       (dvoid *) &x2world[0],
                                        (sb4) sizeof(double), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 25,
                                       (dvoid *) &x2errworld[0],
                                        (sb4) sizeof(double), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 26,
                                       (dvoid *) &y2world[0],
                                        (sb4) sizeof(double), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 27,
                                       (dvoid *) &y2errworld[0],
                                        (sb4) sizeof(double), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 28,
                                       (dvoid *) &xyworld[0],
                                        (sb4) sizeof(double), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 29,
                                       (dvoid *) &xyerrworld[0],
                                        (sb4) sizeof(double), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 30,
                                       (dvoid *) &threshold[0],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 31,
                                       (dvoid *) &ximage[0],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 32,
                                       (dvoid *) &yimage[0],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 33,
                                       (dvoid *) &xminimage[0],
                                        (sb4) sizeof(int), SQLT_INT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(int), indsk, rlsk,
                                               rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 34,
                                       (dvoid *) &yminimage[0],
                                        (sb4) sizeof(int), SQLT_INT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(int), indsk, rlsk,
                                               rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 35,
                                       (dvoid *) &xmaximage[0],
                                        (sb4) sizeof(int), SQLT_INT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(int), indsk, rlsk,
                                               rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 36,
                                       (dvoid *) &ymaximage[0],
                                        (sb4) sizeof(int), SQLT_INT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(int), indsk, rlsk,
                                               rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 37,
                                       (dvoid *) &x2image[0],
                                        (sb4) sizeof(double), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 38,
                                       (dvoid *) &x2errimage[0],
                                        (sb4) sizeof(double), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 39,
                                       (dvoid *) &y2image[0],
                                        (sb4) sizeof(double), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 40,
                                       (dvoid *) &y2errimage[0],
                                        (sb4) sizeof(double), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 41,
                                       (dvoid *) &xyimage[0],
                                        (sb4) sizeof(double), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 42,
                                       (dvoid *) &xyerrimage[0],
                                        (sb4) sizeof(double), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(double), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 43,
                                       (dvoid *) &aimage[0],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 44,
                                       (dvoid *) &aerrimage[0],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 45,
                                       (dvoid *) &bimage[0],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 46,
                                       (dvoid *) &berrimage[0],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 47,
                                       (dvoid *) &thetaimage[0],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 48,
                                       (dvoid *) &thetaerrimage[0],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 49,
                                       (dvoid *) &ellipticity[0],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 50,
                                       (dvoid *) &class[0],
                                        (sb4) sizeof(float), SQLT_FLT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(float), indsk,
                                               rlsk, rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                       ctxptr->errhp, (ub4) 51,
                                       (dvoid *) &fflags[0],
                                        (sb4) sizeof(short), SQLT_INT,
                                        (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                        (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
            __LINE__);
  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
                                                (ub4)sizeof(short), indsk,
                                               rlsk, rcsk), __LINE__);


  OCIStmtExecute(ctxptr->svchp, ctxptr->stmthp, ctxptr->errhp, (ub4) (nrows-1),
                 (ub4) 0, (CONST OCISnapshot*) 0, (OCISnapshot*) 0,
                  (ub4) OCI_BATCH_ERRORS);
}




#undef MAXLISTS
#undef MAXIMAGE
#undef SQLLDR_LIMIT
#undef LEN_FIELDS
#undef NUM_FIELDS

