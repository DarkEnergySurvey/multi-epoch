#ifndef DESW_H_
#define DESW_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oci.h>
#include <oratypes.h>

#define DATA_SIZE 5000
#define PIECE_SIZE 1000
#define MAXCOLS 2
#define MAXROWS 10
#define NPIECE DATA_SIZE/PIECE_SIZE
#define MAX_IN_ROWS 10

#define MAXBINDS 500

#ifndef externref
#define externref extern
#endif

/**
 *** Basic OCI handles
 **/
typedef struct desw_ctx
{
	OCIEnv     *envhp;
	OCIServer  *srvhp;
	OCISvcCtx  *svchp;
	OCIError   *errhp;
	OCISession *authp;
	OCIStmt    *stmthp;
} desw_ctx;

struct r_set
{
	char *column[500];
	char *value[500];
	ub4   parmcnt;

	text *pcoln[200];   /* column name: OCI_ATTR_NAME */
	ub2   pcoll[200];   /* column size: OCI_ATTR_DATA_SIZE */
	ub2   podt[200];    /* data type:  OCI_ATTR_DATA_TYPE */
	ub1   isnull[200];  /* is this column NULL? */
	ub4   namelen[200];
};
typedef struct r_set r_set;

/*---------------------------------------------------------------------------
 PRIVATE TYPES AND CONSTANTS
 ---------------------------------------------------------------------------*/
OCIBind *bndhpArray[MAXBINDS];

/*---------------------------------------------------------------------------
 STATIC FUNCTION DECLARATIONS
 ---------------------------------------------------------------------------*/
extern void initialize(desw_ctx *ctxptr, text *username, text *password, text *dbname);
extern void cleanup_db(desw_ctx *ctxptr);
extern void sql_select_execute(desw_ctx *ctxptr, text *stmt,
		r_set *result_set);
extern void sql_stmt_execute(desw_ctx *ctxptr, text *stmt);
extern sword fetch(desw_ctx *ctxptr, r_set *result_set);

#endif /*DESW_H_*/

