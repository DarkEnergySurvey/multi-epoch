/*
/*	Carries out a series of copies from a temporary nightly object table
/*	into the real objects table
/*	Drops and purges the temporary table at the end.
*/


#include "imageproc.h"
#include <unistd.h>

#define COPY_LIMIT 200000

main(argc,argv)
	int argc;
	char *argv[];
{
	char	tmptable[100],sqlquery[500],nite[100],
		dblogin[500],line[1000],sqlcall[1000],
		sqlinsert[1000];
	int	i,flag_quiet=0,num_objects,num_tmptable,min,max,
		min_object_id,max_object_id,totobjects,ntable,count_num,
		flag_no_index_build=0,current_object_id,counter=0,
		objectcount=0,min_object_id_input,flag_min_object_id=0,
		count_min_id,count_max_id;
	FILE	*pip,*out;
	void	select_dblogin(),grab_tableinfo(),count_objects();

  	if ( argc < 2 ) {
    	  printf ("Usage: %s <nite>\n",argv [0]);
	  printf("  -quiet\n");
    	  exit(0);
  	}
	sprintf(nite,"%s",argv[1]);
	

	/* ************************************************************* */
	/* *************  process the command line ********************* */
	/* ************************************************************* */
	for (i=1;i<argc;i++) {
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	} 


	/* ************************************************************* */
	/* *****************  db setup information ********************* */
	/* ************************************************************* */
	/* set up defaults */
	sprintf(sqlquery,"copy_objects_%s.sql",nite);
	sprintf(tmptable,"tmp_objects_%s",nite);
	/* access dblogin */
	select_dblogin(dblogin);
	/* prepare the call */
	sprintf(sqlcall,"${ORACLE_HOME}/bin/sqlplus -S %s < %s",dblogin,
	  sqlquery);
	sprintf(sqlinsert,"${ORACLE_HOME}/bin/sqlplus -S %s < %s > /dev/null",dblogin,
	  sqlquery);

	/* ************************************************************* */
	/* ********** now query for size of temporary table  *********** */
	/* ************************************************************* */

	out=fopen(sqlquery,"w");
        fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP '|'");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
        /* first find out how many objects in tmptable */
        fprintf(out,"SELECT count(*) FROM dba_tables where table_name=UPPER('%s');\n",tmptable);
        fprintf(out,"SELECT count(*) FROM dba_indexes where table_name=UPPER('%s') and index_name=UPPER('OBJID_%s');\n",tmptable,nite);
        fprintf(out,"SELECT count(*) FROM %s;\n",tmptable);
        fprintf(out,"exit;\n");
        fclose(out); 

	if (!flag_quiet) {
	  printf("  Confirming table and getting table count\n");
	  fflush(stdout);
	}

	/* now make the call and read in image information */
	pip=popen(sqlcall,"r");
	fgets(line,1000,pip);
	sscanf(line,"%d",&ntable);
	if (ntable!=1) {
	  printf("  ** copy_objects:  temporary table %s nonexistent\n",
	    tmptable);
	  exit(0);
	}
	fgets(line,1000,pip);
	sscanf(line,"%d",&flag_no_index_build);
	if (!flag_quiet) {
	  if (flag_no_index_build) 
	    printf("  object_id index in %s already exists\n",tmptable);
	  fflush(stdout);
	}
	fgets(line,1000,pip);
	sscanf(line,"%d",&totobjects);
	if (!flag_quiet) {
	  printf("  %d objects in %s\n",totobjects,tmptable);
	  fflush(stdout);
	}
	if (totobjects<-0 || totobjects>1000000000) {
	  exit(0);
	}
	pclose(pip);

	/* ************************************************************* */
	/* ********     Build object_id index in tmptable    *********** */
	/* ********  Determine range of objectid in tmptable *********** */
	/* ************************************************************* */
	
	if (!flag_quiet) {
 	  if (!flag_no_index_build) 
	    printf("  Building index on object_id\n");
	  printf("  Extracting object_id range from %s\n",tmptable);
	  fflush(stdout);
	}
	out=fopen(sqlquery,"w");
        fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP '|'");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	if (!flag_no_index_build) 
          fprintf(out,"CREATE INDEX OBJID_%s on %s(object_id) parallel 4 nologging;\n",
	    nite,tmptable);
        fprintf(out,"SELECT min(object_id) FROM %s;\n",tmptable);
        fprintf(out,"SELECT max(object_id) FROM %s;\n",tmptable);
        fprintf(out,"exit;\n");
        fclose(out); 

	/* now read min and max object_id */
	pip=popen(sqlcall,"r");
	fgets(line,1000,pip);
	sscanf(line,"%d",&min_object_id);
	fgets(line,1000,pip);
	sscanf(line,"%d",&max_object_id);
	pclose(pip);
	if (!flag_quiet) {
	  printf("  Min/Max object_id are %d/%d\n",min_object_id,
	    max_object_id);
	  printf("  Determining status of table copy\n");
	  fflush(stdout);
	}
	grab_tableinfo(sqlquery,sqlcall,tmptable,min_object_id,
	  max_object_id,&objectcount,&current_object_id);
	if (objectcount>0) min_object_id=current_object_id+1;
	if (!flag_quiet) {
	  printf("  %d/%d objects copied; starting object_id:  %d\n",
	    objectcount,totobjects,min_object_id);
	  printf("  Now copying rows from %s into OBJECTS table %d at a time\n\n  ",
	    tmptable,COPY_LIMIT);
	  fflush(stdout);
	}

	/* ************************************************************* */
	/* ***********  Copy objects from tmptable to OBJECTS ********** */
	/* ************************************************************* */
	
	min=min_object_id;
	while (min<max_object_id) {
	  max=min+COPY_LIMIT;  
	  if (max>max_object_id) max=max_object_id+1;

	  /* write query file */
	  out=fopen(sqlquery,"w");
          fprintf(out,"INSERT INTO OBJECTS SELECT * FROM %s WHERE\n",
	    tmptable);
          fprintf(out,"%s.object_id>=%d and %s.object_id<%d;\n",
	    tmptable,min,tmptable,max);
          fprintf(out,"commit;\n");
          fprintf(out,"exit;\n");
          fclose(out); 
	  system(sqlinsert);

	  /* report progress */
	  if (!flag_quiet) {
	    printf(".");
	    fflush(stdout);
	  }
	  min+=COPY_LIMIT;
	  counter++;
	  if (counter%50==0) {
	    count_objects(sqlquery,sqlcall,tmptable,count_min_id,max,&count_num);
	    objectcount+=count_num;
	    if (!flag_quiet) printf("\n%d/%d copied: object_id %d/%d ",
	      objectcount,totobjects,min,max_object_id);
	    count_min_id=max;
	  }
	}

	/* ************************************************************* */
	/* ***************** Drop and purge tmptable ******************* */
	/* ************************************************************* */
	out=fopen(sqlquery,"w");
        fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP '|'");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
        /*fprintf(out,"DROP TABLE %s PURGE;\n",tmptable);*/
        fprintf(out,"SELECT count(*) FROM dba_tables where table_name=UPPER('%s');\n",tmptable);
        fprintf(out,"exit;\n");
        fclose(out); 

	pip=popen(sqlcall,"r");
	fgets(line,1000,pip);
	sscanf(line,"%d",&ntable);
	pclose(pip);
	if (ntable!=0) {
	  printf(" ** copy_objects:  %s still present\n",tmptable);
	  exit(0);
	}
	
}
void	count_objects(sqlquery,sqlcall,table,min,max,count)
	char sqlquery[],sqlcall[],table[];
	int *count,min,max;
{
	FILE	*out,*pip;
	char	line[1000];
	int	i;

	out=fopen(sqlquery,"w");
        fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP ' '");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
        fprintf(out,"SELECT count(*) FROM %s where\n",table);
        fprintf(out,"object_id>=%d and object_id<=%d;\n",
	  min,max);
        fprintf(out,"exit;\n");
        fclose(out); 
	/* execute query and read feedback */
	pip=popen(sqlcall,"r");
	fgets(line,1000,pip);
	sscanf(line,"%d %d",count);
	pclose(pip);
}



void	grab_tableinfo(sqlquery,sqlcall,table,min,max,count,max_object_id)
	char sqlquery[],sqlcall[],table[];
	int *count,*max_object_id,min,max;
{
	FILE	*out,*pip;
	char	line[1000];
	int	i;

	out=fopen(sqlquery,"w");
        fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP ' '");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
        fprintf(out,"SELECT count(*),max(%s.object_id) FROM objects,%s where\n",table,table);
        fprintf(out,"objects.object_id=%s.object_id\n",table);
        fprintf(out,"and objects.object_id>=%d and objects.object_id<=%d;\n",
	  min,max);
        fprintf(out,"exit;\n");
        fclose(out); 
	/* execute query and read feedback */
	pip=popen(sqlcall,"r");
	fgets(line,1000,pip);
	sscanf(line,"%d %d",count,max_object_id);
	pclose(pip);
}
#undef COPY_LIMIT
