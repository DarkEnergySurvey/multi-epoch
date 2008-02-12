/* 
   run at this level:
   runCatalog_ingest.c  -- wrapper program for running
   catalog_ingest, a program that reads FITS catalogs and ingests them
   into the Archive

   runid/data/nite/band/imagename/imagename_cat.fits
*/
#include "imageproc.h"
#include <unistd.h>

#include "desw_rel.h"

#define MAXLISTS 500
#define MAXIMAGE 100
#define SQLLDR_LIMIT 50000
#define LEN_FIELDS 100
#define NUM_FIELDS 9
#define STRING_FIELDS (LEN_FIELDS*NUM_FIELDS)

#define LINK "dber.ncsa.uiuc.edu/sony"
#define USER "sony_admin"
#define PASS "sonymgr"

typedef unsigned long long uint64;

int htmdepth=20;

//fitsfile *fptr;      /* FITS file pointer, defined in fitsio.h */
char *val, value[1000], nullstr[]="*";
char keyword[FLEN_KEYWORD], colname[FLEN_VALUE], comment[500];
//int status = 0;   /*  CFITSIO status value MUST be initialized to zero!  */
int hdunum, hdutype, ncols, ii, anynul, dispwidth[1000], parentid, softid;
int i,firstcol= 1, lastcol, linewidth, imageid,flag_ascii=0;
long jj, nrows;
float magzp,equinox=2000.0;
float *magaper,*magerrap,*magauto,*magerrauto,*threshold,*ximage;
float *yimage,*aimage,*aerrimage,*bimage,*berrimage,*thetaimage;
float *thetaerrimage,*ellipticity,*class,floatnull=0;
int *objnum,*xminimage,*yminimage,*xmaximage,*ymaximage;
short *fflags,shortnull=0;
int **pointer,**nullpointer, j,k,*typecode,intnull=0;
char **prstring;
long *repeat,*width,longnull=0;
double doublenull=0.0,*alphara,*deltadec,*alphapeak,*deltapeak,*x2world;
double *x2errworld,*y2world,*y2errworld,*xyworld,*xyerrworld,*x2image;
double *x2errimage,*y2image,*y2errimage,*xyimage,*xyerrimage;
char band[5], command[1000],line[1000];
double *cx,*cy,*cz;


void getxyz(), set_piece(), insert_objects();
uint64 cc_radec2ID();
uint64 *htmID;

/* for ascii input */
FILE  *pip,*fin;
int objnum_ascii;
double magauto_ascii, magerrauto_ascii;
double magaper1_ascii, magerrap1_ascii, magaper2_ascii, magerrap2_ascii, magaper3_ascii, magerrap3_ascii, magaper4_ascii, magerrap4_ascii, magaper5_ascii, magerrap5_ascii, magaper6_ascii, magerrap6_ascii;
double alphara_ascii, deltadec_ascii, alphapeak_ascii, deltapeak_ascii;
double x2world_ascii, x2errworld_ascii, y2world_ascii, y2errworld_ascii, xyworld_ascii;
double xyerrworld_ascii, threshold_ascii, ximage_ascii, yimage_ascii;
int xminimage_ascii, yminimage_ascii, xmaximage_ascii, ymaximage_ascii;
double x2image_ascii, x2errimage_ascii, y2image_ascii, y2errimage_ascii;
double xyimage_ascii, xyerrimage_ascii, aimage_ascii, aerrimage_ascii;
double bimage_ascii, berrimage_ascii, thetaimage_ascii, thetaerrimage_ascii;
double ellipticity_ascii, class_ascii;
int fflags_ascii;
desw_ctx ctx;

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
    printf("  -sqlldr_ctrl <file>\n");
    printf("  -sqlldr_data <file>\n");
    printf("  -sqlldr_multiingest <#>\n");
    printf("  -ccd <#>\n");
    printf("  -quiet\n");
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

  initialize(&ctx, (text *)USER, (text *)PASS, (text *)LINK);

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
        printf(
               "%s\n runid=%s nite=%s band=%s tilename=%s imagetype=%s ccdnum=%d newimagename=%s\n",
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

  /* ************************************************************* */
  /* ****** now query for list of images with these runids ******* */
  /* ************************************************************* */

  sprintf(filename, "%s.sqlquery", sqlldr_root);
  out=fopen(filename, "w");
  fprintf(out, "SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP '|'");
  fprintf(out, "HEAD OFF TRIMS ON LINESIZE 1000;\n");
  /* first find out how many solutions to expect */
  fprintf(out, "SELECT count(*)\n");
  fprintf(out, "FROM FILES\n");
  fprintf(out, "WHERE RUNIDDESC='%s' ", runids[0]);
  for (i=1; i<nrunids; i++)
    fprintf(out, "\nOR RUNIDDESC='%s'", runids[i]);
  fprintf(out, ";\n");
  fprintf(
          out,
          "SELECT imageid||'|'||nite||'|'||band||'|'||tilename||'|'||runiddesc||'|'||imagename||'|'||imagetype||'|'||ccd_number||'|'||equinox \n");
  fprintf(out, "FROM FILES\n");
  fprintf(out, "WHERE RUNIDDESC='%s' ", runids[0]);
  for (i=1; i<nrunids; i++)
    fprintf(out, "\nOR RUNIDDESC='%s'", runids[i]);
  fprintf(out, ";\n");
  fprintf(out, "exit;\n");
  fclose(out);

  sprintf(
          sqlcall,
          "${ORACLE_HOME}/bin/sqlplus -S pipeline/dc01user@charon.ncsa.uiuc.edu/des < %s.sqlquery",
          sqlldr_root);

  /* now make the call and read in image information */
  i=-1;
  pip=popen(sqlcall, "r");
  while (fgets(line, 1000, pip)!=NULL)
  {
    if (i==-1)
    {
      sscanf(line, "%d", &nfiles);
      if (!flag_quiet)
        printf("  Selected %d files from db\n", nfiles);
      if (nfiles==0)
      {
        printf("  ** runCatalog_ingest:  Query must select at least one file\n");
        exit(0);
      }
      im=(struct imageinfo *)calloc(nfiles, sizeof(struct imageinfo));
    }
    else
    {
      /* split the string */
      //printf(" ** %s",line);
      splitstring(line, "|", NUM_FIELDS, LEN_FIELDS, templist);
      /* copy substrings into proper variables */
      sscanf(&(templist[0*LEN_FIELDS]), "%d", &(im[i].imageid));
      sscanf(&(templist[1*LEN_FIELDS]), "%s", im[i].nite);
      sscanf(&(templist[2*LEN_FIELDS]), "%s", im[i].band);
      sscanf(&(templist[3*LEN_FIELDS]), "%s", im[i].tilename);
      sscanf(&(templist[4*LEN_FIELDS]), "%s", im[i].runid);
      sscanf(&(templist[5*LEN_FIELDS]), "%s", im[i].imagename);
      sscanf(&(templist[6*LEN_FIELDS]), "%s", im[i].imagetype);
      sscanf(&(templist[7*LEN_FIELDS]), "%d", &(im[i].ccdnum));
      sscanf(&(templist[8*LEN_FIELDS]), "%f", &(im[i].equinox));
      if (!flag_quiet)
        printf(
               "  imageid: %d nite: %s band: %s tilename: %s runid: %s imagename: %s imagetype: %s ccdnum: %d\n",
               im[i].imageid, im[i].nite, im[i].band, im[i].tilename,
               im[i].runid, im[i].imagename, im[i].imagetype, im[i].ccdnum);
    }
    i++;
    if (i>nfiles)
    {
      printf("  **runCatalog_ingest:  too many files returned from db query \n");
      exit(0);
    }
  }
  pclose(pip);

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
              printf(
                     "  **runCatalog_ingest:  more than one image match found for catalog %s:  %d (imageid=%d) and %d (imageid=%d)\n",
                     imagename, match, im[match].imageid, i, im[i].imageid);
              exit(0);
            }
          }
        }
        if (match<0)
        {
          printf(
                 "\n  **runCatalog_ingest:  no matching image file found for %s\n",
                 imagename);
          exit(0);
        }
        char im_name[5000];
        sprintf(im_name, "/home2/Archive/%s", imagename);

        do_insert(im_name, im[match].imageid, band, im[match].equinox);

      }
    } while (fileread!=EOF);/* completed processing of single catalog */
    fclose(fin);
  } /* completed processing of a single list */
  if (!flag_quiet)
    printf(
           "  runCatalog_ingest complete:  %d objects ingested in %d batches\n",
           num_tot, ingest_count-1);

  cleanup_db(&ctx);

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

  sprintf(
          query,
          "${ORACLE_HOME}/bin/sqlplus -S pipeline/dc01user@charon.ncsa.uiuc.edu/des < %s",
          queryfile);
  pip=popen(query, "r");
  fscanf(pip, "%d", &numjobs);
  pclose(pip);
  return (numjobs);
}

do_insert( imagename, imageid, band, equinox )
char *imagename;
int   imageid;
char *band;
float equinox;
{
  r_set rs1;

  fitsfile *fptr;

  int status = 0;

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
    for (i=1; i<ncols; i++)
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

  insert_objects(&ctx);
}

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

  printf("INSERT: %s\n", instStmt);

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
                                        (dvoid *) &objnum[0],
                                       (sb4) sizeof(int), SQLT_INT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
           __LINE__);
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

