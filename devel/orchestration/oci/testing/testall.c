#include "desw_rel.h"

#define LINK "dber.ncsa.uiuc.edu/sony"
#define USER "sony_admin"
#define PASS "sonymgr"

int main(argc, argv)
  int argc;
  char *argv[];
{
  desw_ctx ctx;
  desw_ctx ctx2;
  sword retval = 0;
  int i;
  int k;

int row;
int row2;

  sword ociret;

  initialize(&ctx, (text *)USER, (text *)PASS, (text *)LINK);
  initialize(&ctx2, (text *)USER, (text *)PASS, (text *)LINK);
  
  r_set rs1;
  r_set rs2;

  printf( "TABLE test:\n" );
  sql_select_execute(&ctx2, (text *)"SELECT * FROM test", &rs2 );

row = 0;
  while( (retval = fetch( &ctx2, &rs2 )) == OCI_SUCCESS ||
      (retval == OCI_SUCCESS_WITH_INFO) )
  {
printf( "[%d]:\t", row++ );
    for (k = 0; k < rs2.parmcnt; k++)
    {
      if (strlen((void *)rs2.value[k]) != 0)
      {
//        printf( "CTX2(%d): ", k );
        printf("%-*s ", rs2.pcoll[k],
            (void *)rs2.value[k],
            strlen((void *)rs2.value[k]));
      }
    }
    printf( "\n" );
  }


  /* execute sql statement */
  printf("\n\n\nDo INSERT ... \n");

  char tmp[500];
  char telescopename[500] = "Blanco 4m";
  char detector[500] = "DECam";
  char tmp2[1000];

  int *ccd;
  float *raoff, *decoff, *rahw, *dechw;
  int j = 0;
  int ccdtotal = 62;

  /**
   sprintf(tmp,"SELECT chipid,raoffset,decoffset,rahwidth,dechwidth FROM wcsoffset WHERE TELESCOPE='%s' and DETECTOR='%s' ORDER BY chipid\n",
   telescopename,detector);
   **/

  /* memory allocatio for the wcsoffset info */
  ccd    = (int *)calloc(ccdtotal+1, sizeof(int));
  raoff  = (float *)calloc(ccdtotal+1, sizeof(float));
  decoff = (float *)calloc(ccdtotal+1, sizeof(float));
  rahw   = (float *)calloc(ccdtotal+1, sizeof(float));
  dechw  = (float *)calloc(ccdtotal+1, sizeof(float));

  sprintf(tmp,
          "insert into test ( X_IMAGE,YMAX_IMAGE,BAND) values (-.300, 5, 'o')");



sql_select_execute(&ctx, (text *)tmp, &rs2 );



  printf( "**** DO FIRST INSERT ****\n");



//return;


  sql_stmt_execute(&ctx, (text *)tmp);
  printf("%s:\n", tmp );

  sprintf(tmp, "SELECT * FROM test");
  sql_select_execute(&ctx, (text *)tmp, &rs1);

  printf( "TEST 1:\n" );

row = 0;
  while( (retval = fetch( &ctx, &rs1 )) == OCI_SUCCESS ||
      (retval == OCI_SUCCESS_WITH_INFO) )
  {
printf( "[%d]:\t", row++ );
    for (i = 0; i < rs1.parmcnt; i++)
    {
//      printf( "i: %d\n", i );
      printf("%-*s ", rs1.pcoll[i],
          (void *)rs1.value[i],
          strlen((void *)rs1.value[i]));

      strcat( tmp2, rs1.value[i] );
      strcat( tmp2, " " );
    }
    printf( "\n" );

    printf( "TEMP2: %s\n\n", tmp2);

    sscanf(tmp2,"%d %f %f %f %f",&ccd[j],&raoff[j],&decoff[j],&rahw[j],&dechw[j]);
    j++;

    tmp2[0] = '\0';
    
    printf( "\n******** DO SECOND INSERT ********\n");
    sprintf(tmp,
            "insert into test ( X_IMAGE,YMAX_IMAGE,BAND) values (.78, 888, 'S')");
    printf( "%s:\n", tmp );
    sql_stmt_execute(&ctx2, (text *)tmp);
    
    sql_select_execute(&ctx2, (text *)"SELECT * FROM test", &rs2 );

    printf( "TEST 2:\n" );

row2 = 0;
    while( (retval = fetch( &ctx2, &rs2 )) == OCI_SUCCESS ||
        (retval == OCI_SUCCESS_WITH_INFO) )
    {
printf( "[%d]:\t", row2++ );
      for (k = 0; k < rs2.parmcnt; k++)
      {
        if (strlen((void *)rs2.value[k]) != 0)
        {
//          printf( "CTX2(%d): ", k );
          printf("%-*s ", rs2.pcoll[k],
              (void *)rs2.value[k],
              strlen((void *)rs2.value[k]));
        }
      }
      printf( "\n" );
    }
  }
  
  
  
  printf( "\n\n**** DO UPDATE ****\n");
  sprintf(tmp, "update test set X_IMAGE = .11, BAND = 'n' where X_IMAGE = .78");
  printf( "%s:\n", tmp );

  sql_select_execute(&ctx2, (text *)"SELECT * FROM test where X_IMAGE = .78", &rs2 );

  printf( "TEST 3 (before update):\n" );

row = 0;
  while( (retval = fetch( &ctx2, &rs2 )) == OCI_SUCCESS ||
      (retval == OCI_SUCCESS_WITH_INFO) )
  {
printf( "[%d]:\t", row++ );
    for (k = 0; k < rs2.parmcnt; k++)
    {
      if (strlen((void *)rs2.value[k]) != 0)
      {
//        printf( "CTX2(%d): ", k );
        printf("%-*s ", rs2.pcoll[k],
            (void *)rs2.value[k],
            strlen((void *)rs2.value[k]));
      }
    }
    printf( "\n" );
  }


  sql_stmt_execute(&ctx, (text *)tmp);
  
  sql_select_execute(&ctx2, (text *)"SELECT * FROM test", &rs2 );

  printf( "TEST 4 (after update):\n" );

row = 0;
  while( (retval = fetch( &ctx2, &rs2 )) == OCI_SUCCESS ||
      (retval == OCI_SUCCESS_WITH_INFO) )
  {
printf( "[%d]:\t", row++ );
    for (k = 0; k < rs2.parmcnt; k++)
    {
      if (strlen((void *)rs2.value[k]) != 0)
      {
//        printf( "CTX2(%d): ", k );
        printf("%-*s ", rs2.pcoll[k],
            (void *)rs2.value[k],
            strlen((void *)rs2.value[k]));
      }
    }
    printf( "\n" );
  }
  
  printf( "\n\n**** DO DELETE ****\n");
  sprintf(tmp, "delete from test where BAND = 'n'");
  printf( "%s:\n", tmp );
  sql_stmt_execute(&ctx, (text *)tmp);
  
  sql_select_execute(&ctx2, (text *)"SELECT * FROM test", &rs2 );
  printf( "TEST 5:\n" );

row = 0;
  while( (retval = fetch( &ctx2, &rs2 )) == OCI_SUCCESS ||
      (retval == OCI_SUCCESS_WITH_INFO) )
  {
printf( "[%d]:\t", row++ );
    for (k = 0; k < rs2.parmcnt; k++)
    {
      if (strlen((void *)rs2.value[k]) != 0)
      {
//        printf( "CTX2(%d): ", k );
        printf("%-*s ", rs2.pcoll[k],
            (void *)rs2.value[k],
            strlen((void *)rs2.value[k]));
      }
    }
    printf( "\n" );
  }

  /* clean things up before exhit */
  cleanup(&ctx);
  cleanup(&ctx2);

  return 1;

} /*end main*/

