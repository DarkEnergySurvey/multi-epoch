#include "imageproc.h"

/* pull db user/passwd@platform/name from protected file */

#define DB_USER "DB_USER"
#define DB_PASSWD "DB_PASSWD"
#define DB_SERVER "DB_SERVER"
#define DB_NAME "DB_NAME"

void select_dblogin(dblogin)
	char *dblogin;
{
	FILE	*inp;
	char	line[1000],filename[1000],db_user[100],db_passwd[100],
		db_server[1000],db_name[100],trash[1000];

	/* define hidden file name  */
	sprintf(filename,"%s/.desdm",getenv("HOME"));
	inp=fopen(filename,"r");
	if (inp==NULL) {
	  printf("** DESDM configuration file %s not found.\n",
	    filename);
	  exit(0);
	}	
	while (fgets(line,1000,inp)!=NULL) {
	  if (!strncmp(line,DB_USER,strlen(DB_USER))) 
	    sscanf(line,"%s %s",trash,db_user);
	  if (!strncmp(line,DB_PASSWD,strlen(DB_PASSWD))) 
	    sscanf(line,"%s %s",trash,db_passwd);
	  if (!strncmp(line,DB_SERVER,strlen(DB_SERVER))) 
	    sscanf(line,"%s %s",trash,db_server);
	  if (!strncmp(line,DB_NAME,strlen(DB_NAME))) 
	    sscanf(line,"%s %s",trash,db_name);
	}
	fclose(inp);

	/* create the db call */
	sprintf(dblogin,"%s/%s@%s/%s",db_user,db_passwd,
	  db_server,db_name);
}

#define MAXNODES 20

/* retrieve the archive node information from the db */
void select_archivenode(dblogin,arnode,arroot,arsites)
	char *dblogin,*arnode,*arroot,*arsites;
{
	FILE	*out,*pip;
	char	filename[100],arhost[100],trash[1000],sqlcall[5000];
	int	arid,n,i,num_nodes;

	/* first produce query */
        sprintf(filename,"select_archivenode.sqlquery");
        out=fopen(filename,"w");
        fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP ' |'");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
        /* first find out how many solutions to expect */
        fprintf(out,"SELECT LOCATION_NAME||'|'||LOCATION_ID||' |'||ARCHIVE_HOST||' |'||ARCHIVE_ROOT FROM ARCHIVE_SITES;\n");
        fprintf(out,"exit;\n");
        fclose(out); 
	/* execute query */
	sprintf(sqlcall,"${ORACLE_HOME}/bin/sqlplus -S %s < %s",
	  dblogin,filename);
	pip=popen(sqlcall,"r");
	printf("  Looking for %s\n    Known archive sites: |",arnode);
	arid=0;
	while (fgets(trash,1000,pip)!=NULL) {
	  i=n=0;
	  while (strncmp("|",trash+i,1)) i++;
	  if (!strncmp(trash,arnode,i-1)) {
	    //if (!strcmp(trash,arnode)) {
	    while (strncmp(trash+n,"|",1)) n++;
	    sscanf(trash+(++n),"%d",&arid);
	    while (strncmp(trash+n,"|",1)) n++;
	    sscanf(trash+(++n),"%s",arhost);
	    while (strncmp(trash+n,"|",1)) n++;
	    sscanf(trash+(++n),"%s",arroot);
	    
	  }
	  else {
	    while (strncmp(trash+n,"|",1)) n++;
	    sscanf(trash+(++n),"%d",&i);
	    if (i>num_nodes) num_nodes=i;
	  }
	  /* print archive site */
	  n=0;while (strncmp(trash+n,"|",1)) {
	    printf("%c",trash[n]);
	    n++;
	  }
	  printf("|");
	}
	pclose(pip);
	if (arid==0) { 
	  printf("\n\n**  Archive node %s unknown\n",arnode);
	  exit(0);
	}

	arsites[0]=0;
	if (arid>num_nodes) num_nodes=arid;
	if (num_nodes>MAXNODES) num_nodes=MAXNODES;
	for (i=0;i<num_nodes;i++) 
	  if (i==arid-1) sprintf(arsites,"%sY",arsites);
	  else sprintf(arsites,"%sN",arsites);

	printf("\n    %s is site %d of %d with root directory %s and archive_site %s\n",
	  arnode,arid,num_nodes,arroot,arsites); 

	/* choong's addition 12/05/2007 */
	system("rm select_archivenode.sqlquery");
	/* end choong's addition 12/05/2007 */
}

/* subroutine that returns the number of active DB jobs with program like 'name' */
int numdbjobs(name,root,dblogin)
        char name[],root[],dblogin[];
{
        FILE    *out,*pip;
        int     numjobs;
        char    queryfile[500],query[500];

        sprintf(queryfile,"%s_numdbjobs.sql",root);
        out=fopen(queryfile,"w");
        fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP '|'");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
        /* first find out how many solutions to expect */
        fprintf(out,"SELECT count(DB_system_id)\n");
        fprintf(out,"FROM DES_ACTIVE_SESSIONs\n");
        fprintf(out,"WHERE program like '\%%");
        fprintf(out,"%s",name);
        fprintf(out,"\%';\n");
        fprintf(out,"exit;\n");
        fclose(out);

        sprintf(query,"${ORACLE_HOME}/bin/sqlplus -S %s < %s",
          dblogin,queryfile);
        pip=popen(query,"r");
        fscanf(pip,"%d",&numjobs);
        pclose(pip);
        return(numjobs);
}

/* subroutine that returns the number of active DB jobs with program like 'name' */
int table_exists(dblogin,dbtable,root)
        char dblogin[],dbtable[],root[];
{
        FILE    *out,*pip;
        int     numtables,i;
        char    queryfile[500],temp[500],dbuser[500],query[500];

	/* pull dbuser from the dblogin string */
	sprintf(temp,"%s",dblogin);
	for (i=0;i<strlen(temp);i++) if (!strncmp(temp+i,"/",1)  || 
	  !strncmp(temp+i,"@",1)) temp[i]=32;
	sscanf(temp,"%s",dbuser);

	/* now check whether table exists */
        sprintf(queryfile,"%s_table_exists.sql",root);
        out=fopen(queryfile,"w");
        fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP '|'");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
        /* first find out how many solutions to expect */
        fprintf(out,"SELECT count(1) from dba_tables\n");
        fprintf(out,"WHERE owner='%s' and table_name='%s';\n",dbuser,dbtable);
	fprintf(out,"exit;\n");
        fclose(out);

        sprintf(query,"${ORACLE_HOME}/bin/sqlplus -S %s < %s",
          dblogin,queryfile);
        pip=popen(query,"r");
        fscanf(pip,"%d",&numtables);
        pclose(pip);
        return(numtables);
}

/* subroutine that returns the number of active DB jobs with program like 'name' */
int create_table(dblogin,dbtable,tabletype,root)
        char dblogin[],dbtable[],tabletype[],root[];
{
        FILE    *out,*pip;
        int     numtables;
        char    queryfile[500],query[500];

        sprintf(queryfile,"%s_create_table.sql",root);
        out=fopen(queryfile,"w");
	if (!strcmp(tabletype,"OBJECTS"))
        fprintf(out,"create table %s as select * from %s where object_id=-9999;\nexit;",
	  dbtable,tabletype);
	else {
	  printf("  Table template %s not found!\n",tabletype);
	  exit(0);
	}
	fclose(out);

        sprintf(query,"${ORACLE_HOME}/bin/sqlplus -S %s < %s",
          dblogin,queryfile);
	system(query);
}


#undef MAXNODES
#undef DB_USER
#undef DB_PASSWD
#undef DB_SERVER
#undef DB_NAME


