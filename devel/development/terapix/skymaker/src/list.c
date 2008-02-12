/*
                                  list.c

*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*
*       Part of:        SkyMaker
*
*       Author:         E.BERTIN (IAP)
*
*       Contents:       Handling of lists of simulated objects.
*
*       Last modify:    17/08/2006
*
*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "define.h"
#include "globals.h"
#include "fft.h"
#include "galaxies.h"
#include "image.h"
#include "list.h"
#include "prefs.h"
#include "simul.h"
#include "stars.h"
#include "random.h"

#ifdef USE_THREADS
#include "threads.h"

static void		pthread_readlist(simstruct *sim),
			*pthread_readobj(void *arg);
static int		pthread_nextobj(int objindex, char *str);

static pthread_t	*thread;
static pthread_mutex_t	objmutex, imagemutex;
extern pthread_mutex_t	dftmutex;
static pthread_cond_t	*objcond;

static simstruct	*pthread_sim;
static objstruct	*pthread_obj;
static char		pthread_str[MAXCHAR];
static int		*pthread_addobjflag, *pthread_objqueue, pthread_endflag,
			pthread_addobji,pthread_procobji, pthread_nobj,
			pthread_objn;
#endif

/****** readlist ************************************************************
PROTO	void readlist(simstruct *sim)
PURPOSE	Read the object list.
INPUT	Pointer to the sim structure.
OUTPUT	-.
NOTES	-.
AUTHOR	E. Bertin (IAP)
VERSION	17/08/2006
 ***/
void    readlist(simstruct *sim)

  {
#ifndef USE_THREADS
   objstruct		obj = {0};
   char			str[MAXCHAR], msg[MAXCHAR];
   int			i;
#endif
/* First test if a list-filename has been provided in the command line */
  if (!(*(sim->inlistname)))
    return;

  NFPRINTF(OUTPUT, "Reading input list...");

  if ((sim->inlistfile = fopen(sim->inlistname,"r")) == NULL)
    error(EXIT_FAILURE,"*ERROR*: can't read ", sim->inlistname);

  fft_init(1);

#ifdef USE_THREADS
  pthread_readlist(sim);
#else
  obj.flux = 0.0;
  for (i=0; fgets(str, MAXCHAR, sim->inlistfile); i++)
    {
    if (readobj(sim, &obj, str, 0) == RETURN_ERROR)
      continue;
    if (!(i%READLIST_DISPSTEP))
      {
      sprintf(msg, "Painting input list... (%d objects)", i);
      NFPRINTF(OUTPUT, msg);
      }
    if (obj.type == 100)
      make_star(sim, &obj);
    else
      make_galaxy(sim, &obj);
/*-- Add the object to the image */
    add_image(obj.subimage, obj.subsize[0], obj.subsize[1],
	sim->image, sim->fimasize[0], sim->fimasize[1],
	obj.subx, obj.suby, (float)obj.subfactor);
/*-- Add the object to the output list */
    writeobj(sim, &obj);
    }
#endif

  fclose(sim->inlistfile);
  fft_end(1);

  return;
  }


/****** readobj ************************************************************
PROTO	int readobj(simstruct *sim, objstruct *obj, char *str, int proc)
PURPOSE	Read the data an object.
INPUT	Pointer to the sim structure,
	pointer to the obj structure,
	character string,
	process index (ignored if not from a thread).
OUTPUT	-.
NOTES	-.
AUTHOR	E. Bertin (IAP)
VERSION	17/08/2006
 ***/
int	readobj(simstruct *sim, objstruct *obj, char *str, int proc)
  {
   char		*cptr, *strtokbuf;

/*-- Examine current input line (discard empty and comment lines) */
  if (!*str || strchr("#\t\n",*str))
    return RETURN_ERROR;
  if (!(cptr=strtok_r(str, " \t", &strtokbuf)))
    return RETURN_ERROR;
  obj->type = atoi(cptr);
  obj->flux = 0.0;
  if (!(cptr=strtok_r(NULL, " \t", &strtokbuf)))
    return RETURN_ERROR;
  obj->x   = atof(cptr);
  if (!(cptr=strtok_r(NULL, " \t", &strtokbuf)))
    return RETURN_ERROR;
  obj->y   = atof(cptr);
/* In FITS, the center of the first pixel is at position (1.0,1.0) */
  obj->x -= 1.0;
  obj->y -= 1.0;
  if (!(cptr=strtok_r(NULL, " \t", &strtokbuf)))
    return RETURN_ERROR;
  obj->mag = atof(cptr);
  if (obj->type == 200)
    {
    obj->bulge_ratio = (cptr=strtok_r(NULL, " \t", &strtokbuf))?
	atof(cptr) : 1.0;
    obj->bulge_req = (cptr=strtok_r(NULL, " \t", &strtokbuf))?
	atof(cptr) : 1.0;
    obj->bulge_ar = (cptr=strtok_r(NULL, " \t", &strtokbuf))?
	atof(cptr) : 1.0;
    obj->bulge_posang = (cptr=strtok_r(NULL, " \t", &strtokbuf))?
	atof(cptr) : random_double(proc)*360.0 - 180.0;
    obj->disk_scale = (cptr=strtok_r(NULL, " \t", &strtokbuf))?
	atof(cptr) : 1.0;
    obj->disk_ar = (cptr=strtok_r(NULL, " \t", &strtokbuf))?
	atof(cptr) : 1.0;
    obj->disk_posang = (cptr=strtok_r(NULL, " \t", &strtokbuf))?
	atof(cptr) : random_double(proc)*360.0 - 180.0;
    obj->z = (cptr=strtok_r(NULL, " \t", &strtokbuf))?
	atof(cptr) : 0.0;
    }

  return RETURN_OK;
  }


/****** writeobj ************************************************************
PROTO	void writeobj(simstruct *sim, objstruct *obj)
PURPOSE	Write the data an object.
INPUT	Pointer to the sim structure,
	pointer to the obj structure.
OUTPUT	-.
NOTES	-.
AUTHOR	E. Bertin (IAP)
VERSION	11/08/2006
 ***/
void    writeobj(simstruct *sim, objstruct *obj)
  {
   char			str[MAXCHAR];

 /*-- The format depends on object type */
  if (obj->type == 100)
    fprintf(sim->outlistfile, "%3d %10.3f %10.3f %8.4f\n",
	obj->type, obj->x+1, obj->y+1, obj->mag);
  else if (obj->type == 200)
    fprintf(sim->outlistfile, "%3d %10.3f %10.3f %8.4f %5.3f %9.3f %5.3f "
				"%+7.2f %9.3f %5.3f %+7.2f %8.5f\n",
	obj->type, obj->x+1, obj->y+1, obj->mag,
	obj->bulge_ratio, obj->bulge_req, obj->bulge_ar, obj->bulge_posang,
	obj->disk_scale, obj->disk_ar, obj->disk_posang, obj->z);
  else
    {
    sprintf(str, "%d", obj->type);
    error(EXIT_FAILURE, "*Error*: Unknown object type in input list: ", str);
    }

  return;
  }


/***************************** openoutlist ***********************************/
/*
Open up the output list for subsequent writings.
*/
void    openoutlist(simstruct *sim)

  {

  if ((sim->outlistfile = fopen(sim->outlistname, "w")) == NULL)
    error(-1,"*ERROR*: can't create ", sim->outlistname);

  return;
  }


/***************************** closeoutlist **********************************/
/*
Open up the output list for subsequent writings.
*/
void    closeoutlist(simstruct *sim)

  {
  fclose(sim->outlistfile);

  return;
  }


/****** endobj ************************************************************
PROTO	void endobj(objstruct *obj)
PURPOSE	Free memory allocated for an object.
INPUT	Pointer to the obj structure.
OUTPUT	-.
NOTES	-.
AUTHOR	E. Bertin (IAP)
VERSION	16/08/2006
 ***/
void	endobj(objstruct *obj)
  {
  if (obj->subsize[0] && obj->subsize[1])
    free(obj->subimage);

  if (obj->buf1size)
    {
    free(obj->maskbuf);
    free(obj->nmaskbuf);
    free(obj->startbuf);
    }
  if (obj->buf2size)
    {
    free(obj->buf2);
    }

  return;
  }

#ifdef USE_THREADS

/****** pthread_readobj ******************************************************
PROTO	void *pthread_readobj(void *arg)
PURPOSE	Object generation thread.
INPUT	Pointer to the thread number.
OUTPUT	NULL void pointer.
NOTES	Relies on global variables.
AUTHOR	E. Bertin (IAP)
VERSION	17/08/2006
 ***/
static void	*pthread_readobj(void *arg)
  {
   objstruct	*obj;
   char		str[MAXCHAR];
   int		obji, proc;

  proc = *((int *)arg);
  obji = -1;
/* Exit if the end of file is reached by any thread, including this one */
  while ((obji=pthread_nextobj(obji, str))!=-1)
    {
    obj = &pthread_obj[obji];
    if (readobj(pthread_sim, obj, str, proc) != RETURN_OK)
      continue;
    if (obj->type == 100)
      make_star(pthread_sim, obj);
    else
      make_galaxy(pthread_sim, obj);
    }

  pthread_exit(NULL);
  return (void *)NULL;
  }


/****** pthread_nextobj ******************************************************
PROTO	int pthread_nextobj(int objindex, char *str)
PURPOSE	Manage object list reading and thread synchronisation.
INPUT	Previous object index in list,
	string buffer.
OUTPUT	Next object index in list.
NOTES	Relies on global variables.
AUTHOR	E. Bertin (IAP)
VERSION	17/08/2006
 ***/
static int	pthread_nextobj(int obji, char *str)
  {
   objstruct	*obj;
   int		q;

  QPTHREAD_MUTEX_LOCK(&objmutex);
/* The newly processed object is ready to be added to the image */
  if (obji>=0)
    pthread_addobjflag[obji] = 2;
/* If we just finished the "right" object, add it to image! */
  if (obji == pthread_addobji)
    {
    while (pthread_addobjflag[pthread_addobji]==2)
      {
      obj = &pthread_obj[pthread_addobji];
/*---- Add the object to the image */
      if (obj->subimage)
        {
        QPTHREAD_MUTEX_LOCK(&imagemutex);
        add_image(obj->subimage, obj->subsize[0], obj->subsize[1],
		pthread_sim->image, pthread_sim->fimasize[0],
		pthread_sim->fimasize[1],
		obj->subx, obj->suby, (float)obj->subfactor);
        QPTHREAD_MUTEX_UNLOCK(&imagemutex);
/*------ Add the object to the output list */
        obj->flux = 0.0;
        writeobj(pthread_sim, obj);
        }
      pthread_addobjflag[pthread_addobji] = 0;
      QPTHREAD_COND_BROADCAST(&objcond[pthread_addobji]);
      pthread_addobji = (pthread_addobji+1)%pthread_nobj;
      }
    }
/* If no more object to process, return a "-1" (meaning exit thread) */
  if (!pthread_endflag && fgets(str, MAXCHAR, pthread_sim->inlistfile))
    {
    if (!(pthread_objn%READLIST_DISPSTEP))
      {
      sprintf(pthread_str, "Painting input list... (%d objects)", pthread_objn);
      NFPRINTF(OUTPUT, pthread_str);
      }
    pthread_objn++;
    obji = pthread_procobji;
    pthread_procobji = (pthread_procobji+1)%pthread_nobj;
/*-- If the next available buffer has not been flushed yet, wait */
    q=++pthread_objqueue[obji];
    while (pthread_addobjflag[obji] || --q)
      QPTHREAD_COND_WAIT(&objcond[obji], &objmutex);
/*-- Set content */
    if (pthread_objqueue[obji])
      pthread_objqueue[obji]--;
    pthread_addobjflag[obji] = 1;
    }
  else
    {
    pthread_endflag = 1;
    obji=-1;
    }
  QPTHREAD_MUTEX_UNLOCK(&objmutex);

  return obji;
  }


/****** pthread_readlist ***************************************************
PROTO	void pthread_readlist(simstruct *sim)
PURPOSE	Read the object list using multithreads.
INPUT	Pointer to the sim structure.
OUTPUT	-.
NOTES	-.
AUTHOR	E. Bertin (IAP)
VERSION	17/08/2006
 ***/
static void	pthread_readlist(simstruct *sim)
  {
   static pthread_attr_t	pthread_attr;
   int				*proc,
				o, p;

/* Number of active threads */
  nproc = prefs.nthreads;
  pthread_nobj = 2*nproc;	/* A margin of 2X for better efficiency */
  QCALLOC(pthread_obj, objstruct, pthread_nobj);
  QCALLOC(pthread_addobjflag, int, pthread_nobj);
  QCALLOC(pthread_objqueue, int, pthread_nobj);
/* Set up multi-threading stuff */
  QMALLOC(objcond, pthread_cond_t, pthread_nobj);
  for (o=0; o<pthread_nobj; o++)
    {
    QPTHREAD_COND_INIT(&objcond[o], NULL);
    }
  QPTHREAD_MUTEX_INIT(&objmutex, NULL);
  QPTHREAD_MUTEX_INIT(&imagemutex, NULL);
  QPTHREAD_MUTEX_INIT(&dftmutex, NULL);
  QMALLOC(proc, int, nproc);
  QMALLOC(thread, pthread_t, nproc);
  QPTHREAD_ATTR_INIT(&pthread_attr);
  QPTHREAD_ATTR_SETDETACHSTATE(&pthread_attr, PTHREAD_CREATE_JOINABLE);
  pthread_sim = sim;
  pthread_endflag = 0;
  pthread_addobji = pthread_procobji = pthread_objn = 0;
/* Start the reading/generation threads */
  for (p=0; p<nproc; p++)
    {
    proc[p] = p;
    QPTHREAD_CREATE(&thread[p], &pthread_attr, &pthread_readobj, &proc[p]);
    }
  for (p=0; p<nproc; p++)
    QPTHREAD_JOIN(thread[p], NULL);
/* Clean up multi-threading stuff */
  QPTHREAD_MUTEX_DESTROY(&objmutex);
  QPTHREAD_MUTEX_DESTROY(&imagemutex);
  QPTHREAD_MUTEX_DESTROY(&dftmutex);
  QPTHREAD_ATTR_DESTROY(&pthread_attr);
  for (o=0; o<pthread_nobj; o++)
    {
    QPTHREAD_COND_DESTROY(&objcond[o]);
    endobj(&pthread_obj[o]);
    }
  free(objcond);
  free(pthread_obj);
  free(pthread_addobjflag);
  free(pthread_objqueue);
  free(proc);
  free(thread);

  return;
  }

#endif

