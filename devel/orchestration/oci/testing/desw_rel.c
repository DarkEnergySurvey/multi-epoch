#include "desw_rel.h"

/*clean up envionment*/
//void cleanup(ctxptr)
void cleanup_db(ctxptr)
desw_ctx *ctxptr;
{
  printf("\n ########## clean up ############ \n");

  if (OCISessionEnd(ctxptr->svchp, ctxptr->errhp, ctxptr->authp, (ub4) 0))
    printf("FAILED: OCISessionEnd()\n");

  printf("Logged off.\n");

  if (OCIServerDetach(ctxptr->srvhp, ctxptr->errhp, (ub4) OCI_DEFAULT))
    printf("FAILED: OCIServerDetach()\n");

  printf("Detached from server.\n");

  printf("Freeing handles ...\n");
  if (ctxptr->stmthp)
    OCIHandleFree((dvoid *) ctxptr->stmthp, (ub4) OCI_HTYPE_STMT);

  if (ctxptr->errhp)
    OCIHandleFree((dvoid *) ctxptr->errhp, (ub4) OCI_HTYPE_ERROR);

  if (ctxptr->srvhp)
    OCIHandleFree((dvoid *) ctxptr->srvhp, (ub4) OCI_HTYPE_SERVER);

  if (ctxptr->svchp)
    OCIHandleFree((dvoid *) ctxptr->svchp, (ub4) OCI_HTYPE_SVCCTX);

  if (ctxptr->authp)
    OCIHandleFree((dvoid *) ctxptr->authp, (ub4) OCI_HTYPE_SESSION);

  if (ctxptr->envhp)
    OCIHandleFree((dvoid *) ctxptr->envhp, (ub4) OCI_HTYPE_ENV);


} /* end cleanup() */

/*check status and print error information*/
void checkerr(errhp, status, line)
OCIError *errhp;
sword status;
int line;
{
  text errbuf[512];
  sb4 errcode = 0;

  switch (status)
  {
    case OCI_SUCCESS:
      break;
    case OCI_SUCCESS_WITH_INFO:
      (void) printf("(%d) Error - OCI_SUCCESS_WITH_INFO\n", line);
      (void) OCIErrorGet((dvoid *)errhp, (ub4) 1, (text *) NULL, &errcode,
                         errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
      (void) printf("(%d) Error - %.*s\n", line, 512, errbuf);
      break;
    case OCI_NEED_DATA:
      (void) printf("(%d) Error - OCI_NEED_DATA\n", line);
      break;
    case OCI_NO_DATA:
      (void) printf("(%d) Error - OCI_NODATA\n", line);
      break;
    case OCI_ERROR:
      (void) OCIErrorGet((dvoid *)errhp, (ub4) 1, (text *) NULL, &errcode,
                         errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
      (void) printf("(%d) Error - %.*s\n", line, 512, errbuf);
      break;
    case OCI_INVALID_HANDLE:
      (void) printf("(%d) Error - OCI_INVALID_HANDLE\n", line);
      break;
    case OCI_STILL_EXECUTING:
      (void) printf("(%d) Error - OCI_STILL_EXECUTE\n", line);
      break;
    case OCI_CONTINUE:
      (void) printf("(%d) Error - OCI_CONTINUE\n", line);
      break;
    default:
      break;
    }
  } /* end checkerr() */

/*initialize envionment and handler*/
void initialize(ctxptr, username, password, dbname)
desw_ctx *ctxptr;
text     *username;
text     *password;
text     *dbname;
{

  if (OCIEnvCreate((OCIEnv **) &ctxptr->envhp, (ub4)OCI_THREADED|OCI_OBJECT,
                   (dvoid *)0, (dvoid * (*)(dvoid *, size_t)) 0, (dvoid * (*)(dvoid *, dvoid *, size_t))0, (void (*)(dvoid *, dvoid *)) 0, (size_t) 0, (dvoid **) 0 ))
    printf("FAILED: OCIEnvCreate()\n");

  printf("\n ######## Connect to server ############# \n");
  printf("USER: %s\nSERVER: %s\n", username, dbname );

  if (OCIHandleAlloc((dvoid *) ctxptr->envhp, (dvoid **) &ctxptr->errhp,
                     (ub4) OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0))
    printf("FAILED: OCIHandleAlloc() on ctxptr->errhp\n");

  if (OCIHandleAlloc((dvoid *) ctxptr->envhp, (dvoid **) &ctxptr->srvhp,
                     (ub4) OCI_HTYPE_SERVER, (size_t) 0, (dvoid **) 0))
    printf("FAILED: OCIHandleAlloc() on ctxptr->srvhp\n");

  if (OCIHandleAlloc((dvoid *) ctxptr->envhp, (dvoid **) &ctxptr->svchp,
                     (ub4) OCI_HTYPE_SVCCTX, (size_t) 0, (dvoid **) 0))
    printf("FAILED: OCIHandleAlloc() on ctxptr->svchp\n");

  if (OCIHandleAlloc((dvoid *) ctxptr->envhp, (dvoid **) &ctxptr->authp,
                     (ub4) OCI_HTYPE_SESSION, (size_t) 0, (dvoid **) 0))
    printf("FAILED: OCIHandleAlloc() on ctxptr->authp\n");

  if (OCIServerAttach(ctxptr->srvhp,
                      ctxptr->errhp,
                      //              (text *) "des", (sb4) strlen((char *) "des"),
                      // (text *) "dber.ncsa.uiuc.edu/sony",
                      (dvoid *)dbname,
                      // (sb4) strlen((char *) "dber.ncsa.uiuc.edu/sony"),
                      (sb4) strlen((char *) dbname),
                      (ub4) OCI_DEFAULT))
    printf("FAILED: OCIServerAttach()\n");

  if (OCIAttrSet((dvoid *) ctxptr->svchp, (ub4) OCI_HTYPE_SVCCTX,
                 (dvoid *) ctxptr->srvhp, (ub4) 0, (ub4) OCI_ATTR_SERVER,
                 ctxptr->errhp))
    printf("FAILED: OCIAttrSet() server attribute\n");

  /*begin log_on part */
  if (OCIAttrSet((dvoid *) ctxptr->authp, (ub4) OCI_HTYPE_SESSION,
                 (dvoid *) username, (ub4) strlen((char *) username),
                 (ub4) OCI_ATTR_USERNAME, ctxptr->errhp))
    printf("FAILED: OCIAttrSet() userid\n");

  if (OCIAttrSet((dvoid *) ctxptr->authp, (ub4) OCI_HTYPE_SESSION,
                 (dvoid *) password, (ub4) strlen((char *) password),
                 (ub4) OCI_ATTR_PASSWORD, ctxptr->errhp))
    printf("FAILED: OCIAttrSet() passwd\n");

  printf("Logging on as %s  ....\n", username);

  checkerr(ctxptr->errhp, OCISessionBegin((dvoid *)ctxptr->svchp,
                                          ctxptr->errhp, ctxptr->authp,
                                          (ub4) OCI_CRED_RDBMS,
                                          (ub4) OCI_DEFAULT ));

  printf("%s logged on.\n", username);

  if (OCIAttrSet((dvoid *) ctxptr->svchp, (ub4) OCI_HTYPE_SVCCTX,
                 (dvoid *) ctxptr->authp, (ub4) 0, (ub4) OCI_ATTR_SESSION,
                 ctxptr->errhp))
    printf("FAILED: OCIAttrSet() session\n");
  /* end log_on part */

  /* alocate stmt handle for sql queries */

  if (OCIHandleAlloc((dvoid *)ctxptr->envhp, (dvoid **) &ctxptr->stmthp,
          (ub4)OCI_HTYPE_STMT, (CONST size_t) 0, (dvoid **) 0))
  printf("FAILED: alloc statement handle\n");

}
/* end initialize() */

sword fetch(ctxptr, result_set)
desw_ctx *ctxptr;
r_set *result_set;
{
  eword i, pos; /* iterators */
  text *column[20];
  OCIDefine *dfnp[200];
  OCIInd testind;

  for (i = 1; i <= result_set->parmcnt; i++)
  {
    column[i-1]
    = (text *) calloc(result_set->pcoll[i-1] + 1, sizeof(utext));

    checkerr(ctxptr->errhp, OCIDefineByPos(ctxptr->stmthp, &dfnp[i-1],
            ctxptr->errhp, (ub4)i, (dvoid *)column[i-1],
            (sb4)((result_set->pcoll[i-1]+1)*2), (ub2)SQLT_STR,
            (sb2 *)&testind, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT) );
  }

  /* checkerr(ctxptr->errhp, OCIStmtFetch(ctxptr->stmthp, */
  sword retval = OCIStmtFetch(ctxptr->stmthp, ctxptr->errhp, (ub4) 1,
      (ub4) OCI_FETCH_NEXT, (ub4) OCI_DEFAULT);

  if (retval == OCI_SUCCESS || retval == OCI_SUCCESS_WITH_INFO)
  {
    for (i = 0; i < result_set->parmcnt; i++)
    {
      result_set->value[i] = strdup(column[i]);
      memset(column[i], 0, (result_set->pcoll[i] + 1)*sizeof(utext));
    }
  }

  /* Ignore NO DATA FOUND error */
  if (retval != OCI_NO_DATA)
  {
    checkerr(ctxptr->errhp, retval);
  }

  return (retval );
}

void sql_select_execute(ctxptr, stmt, result_set)
desw_ctx *ctxptr;
text     *stmt;
r_set    *result_set;
{
  text *pcoln[200]; /* column name: OCI_ATTR_NAME */
  ub2 pcoll[200]; /* column size: OCI_ATTR_DATA_SIZE */
  ub2 podt[200]; /* data type:  OCI_ATTR_DATA_TYPE */
  ub1 isnull[200]; /* is this column NULL? */
  ub4 namelen[200];

  eword i, pos; /* iterators */
  ub4 parmcnt = 0; /* parameter count */
  sword retval = 0; /* return value from OCI functions */

  OCIDefine *dfnp[200]; /* define handle pointer */
  OCIParam *parmdp; /* a parameter handle */

  OCIInd testind;

  checkerr(ctxptr->errhp, OCIStmtPrepare(ctxptr->stmthp, ctxptr->errhp, stmt,
                                         (ub4) strlen((char *)stmt),
                                         (ub4) OCI_NTV_SYNTAX,
                                         (ub4) OCI_DEFAULT));

  checkerr(ctxptr->errhp, OCIStmtExecute(ctxptr->svchp, ctxptr->stmthp,
                                         ctxptr->errhp, (ub4) 0, (ub4)0,
                                         (OCISnapshot *) NULL,
                                         (OCISnapshot *) NULL, OCI_DEFAULT));

  checkerr(ctxptr->errhp, OCIAttrGet((dvoid *)ctxptr->stmthp,
                                     (ub4)OCI_HTYPE_STMT,
                                     (dvoid *)&result_set->parmcnt, (ub4 *) 0,
                                     (ub4)OCI_ATTR_PARAM_COUNT, ctxptr->errhp));

  for (pos = 1; pos <= result_set->parmcnt; pos++)
  {
    retval = OCIParamGet((dvoid *)ctxptr->stmthp, (ub4)OCI_HTYPE_STMT,
                         ctxptr->errhp, (dvoid **)&parmdp, (ub4) pos );

    if (retval)
    {
      printf("OCIParamGet RC=%d, position=%d\n", retval, pos);
      continue;
    }

    /* get the column name */
    checkerr(ctxptr->errhp, OCIAttrGet((dvoid*) parmdp, (ub4) OCI_DTYPE_PARAM,
    /*(dvoid*) &pcoln[pos-1],*/
    (dvoid*) &result_set->pcoln[pos-1], (ub4 *) &namelen[pos-1],
                                       (ub4) OCI_ATTR_NAME,
                                       (OCIError *) ctxptr->errhp));

    checkerr(ctxptr->errhp, OCIAttrGet((dvoid*) parmdp, (ub4) OCI_DTYPE_PARAM,
                                       (dvoid*) &result_set->pcoll[pos-1],
                                       (ub4 *) 0, (ub4) OCI_ATTR_DATA_SIZE,
                                       (OCIError *) ctxptr->errhp));

    /* get the data type */
    checkerr(ctxptr->errhp, OCIAttrGet((dvoid*) parmdp, (ub4) OCI_DTYPE_PARAM,
                                       (dvoid*) &result_set->podt[pos-1],
                                       (ub4 *) 0, (ub4) OCI_ATTR_DATA_TYPE,
                                       (OCIError *) ctxptr->errhp));

    /* is column null */
    checkerr(ctxptr->errhp, OCIAttrGet((dvoid*) parmdp, (ub4) OCI_DTYPE_PARAM,
                                       (dvoid*) &result_set->isnull[pos-1],
                                       (ub4 *) 0, (ub4) OCI_ATTR_IS_NULL,
                                       (OCIError *) ctxptr->errhp));
  }

  return;
}

/*perform simple insert using LONG API*/
void sql_stmt_execute(ctxptr, stmt)
desw_ctx *ctxptr;
text     *stmt;
{
  checkerr(ctxptr->errhp, OCIStmtPrepare(ctxptr->stmthp, ctxptr->errhp, stmt,
                                         (ub4) strlen((char *)stmt),
                                         (ub4) OCI_NTV_SYNTAX,
                                         (ub4) OCI_DEFAULT),
                                         __LINE__);

//  printf("\nBEGINING %s... \n", stmt);

  checkerr(ctxptr->errhp, OCIStmtExecute(ctxptr->svchp, ctxptr->stmthp,
                                         ctxptr->errhp, (ub4) 1, (ub4)0,
                                         (OCISnapshot *) NULL,
                                         (OCISnapshot *) NULL, OCI_DEFAULT),
                                         __LINE__);

//printf( "COMMIT..\n" );
checkerr(ctxptr->errhp, 
         OCITransCommit( ctxptr->svchp, ctxptr->errhp, (ub4) 0),
         __LINE__ );
}

