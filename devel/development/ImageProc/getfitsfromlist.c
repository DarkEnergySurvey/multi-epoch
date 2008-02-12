#include <stdlib.h>
#include <stdio.h>
#include <string.h>

main(int argc, char *argv [])
{
  char listname[800],fitsname[800];
  int i,count=0,num=0;
  int flag=0,flag_gz=0;

  FILE *fin, *fout;

  if ( argc < 2 ) {
    printf("Usage: %s <input_list> \n", argv [0]);
    printf("       Option:\n");
    printf("              -num <fits_num> \n");
    printf("              -gz (for compressed raw fits files)\n");
    exit(0);
  }
 
  sprintf(listname, "%s", argv[1]);

  /* process the command line */
  for (i=2;i<argc;i++) {
    if (!strcmp(argv[i],"-gz")) flag_gz=1;
    if (!strcmp(argv[i],"-num")) 
      { 
	flag=1;
	num = atoi(argv[i+1]);
      }
  }
	
  if ( (fin=fopen (listname, "r")) == NULL ) {
    printf ("file \"%s\" not found.  Aborting\n", listname);
    exit (0);
  }
 
  if(flag)
    fout = fopen("fitsname.in", "w");
 
  count=0;
  while ( fscanf (fin, "%s", fitsname) != EOF )
    {   
      
      if(flag)
	{
	  if(count == num) {
	    if(flag_gz)
	      fprintf(fout, "%s.gz\n",fitsname);
	    else
	      fprintf(fout, "%s\n",fitsname);
	  }
	}
      
      count++;
    }

  printf("%d\n", count);

  fclose(fin);
  if(flag)
    fclose(fout);

  return (0);
}
  
