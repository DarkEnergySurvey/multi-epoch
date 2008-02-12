#include "Chunk.h"
#include <cmath>
#include <string>
#include <iostream>
#include <assert.h>

#ifndef NOFITS
#include "gary_chunks.h"
#endif

inline void myerror(const char *s,const char *s2 = "")
{
  std::cerr << "Error: " << s << ' ' << s2 << std::endl;
  exit(1);
}

double Chunk::Interpolate(double x, double y) const
{
  assert(x>=0. && x< maxi+1.);
  assert(y>=0. && y< maxj+1.);
  int i = int(floor(x-0.5));
  double dx = x - (i+0.5);
  int j = int(floor(y-0.5));
  double dy = y - (j+0.5);
  assert(dx >= 0. && dx < 1.);
  assert(dy >= 0. && dy < 1.);

/* 
   2    3
     x      the point (x,y) is within square of points 0,1,2,3 as shown
    
   0    1 

  Since the points are really the values at the center of the pixel, it
  is possible for x to fall closer to the edge of the chip than any points.
  In this case, the function does an extrapolation given the nearest
  square of pixels.
*/
  if (i==int(maxi)) {i--; dx += 1.;}
  if (j==int(maxj)) {j++; dy += 1.;}
  if (i==-1) {i++; dx -= 1.;}
  if (j==-1) {j++; dy -= 1.;}
  assert(i>=0 && j>=0 && i+1<=int(maxi) && j+1<=int(maxj));

  double f0 = Get(i,j);
  double f1 = Get(i+1,j);
  double f2 = Get(i+1,j);
  double f3 = Get(i+1,j+1);
  double dfdx = f1-f0;
  double dfdy = f2-f0;
  double d2fdxdy = f3+f0-f1-f2;
  return f0 + dfdx*dx + dfdy*dy + d2fdxdy*dx*dy;
}
  
double Chunk::QuadInterpolate(double x, double y) const
{
  static size_t count=0;
  ++count;
  assert(x>=0. && x< maxi+1.);
  assert(y>=0. && y< maxj+1.);
  size_t i = size_t (floor(x));
  double dx = x - (i+0.5);
  size_t j = size_t (floor(y));
  double dy = y - (j+0.5);
  assert(i<=maxi);
  assert(j<=maxj);
  assert (fabs(dx) <= 0.5);
  assert (fabs(dy) <= 0.5);

/*
  7   4   8

       x          (x,y) is closer to point 0 than any other.
  1   0   2 


  5   3   6

  If any points are off the edge, we set them to the value they would
  have if the second derivative were 0 there.
*/
  double f0 = Get(i,j);
  double f1 = (i > 0) ? Get(i-1,j) : 0.;
  double f2 = (i < maxi) ? Get(i+1,j) : 0.;
  double f3 = (j > 0) ? Get(i,j-1) : 0.;
  double f4 = (j < maxj) ? Get(i,j+1) : 0.;
  double f5 = (i > 0 && j > 0) ? Get(i-1,j-1) : 0.;
  double f6 = (i < maxi && j > 0) ? Get(i+1,j-1) : 0.;
  double f7 = (i > 0 && j < maxj) ? Get(i-1,j+1) : 0.;
  double f8 = (i < maxi && j < maxj) ? Get(i+1,j+1) : 0.;
  if (i == 0) {
    f1 = 2*f0 - f2;
    f5 = 2*f3 - f6;
    f7 = 2*f4 - f8;
  }
  if (i == maxi) {
    f2 = 2*f0 - f1;
    f6 = 2*f3 - f5;
    f8 = 2*f4 - f7;
  }
  if (j == 0) {
    f3 = 2*f0 - f4;
    f5 = 2*f1 - f7;
    f6 = 2*f2 - f8;
  }
  if (j == maxj) {
    f4 = 2*f0 - f3;
    f7 = 2*f1 - f5;
    f8 = 2*f2 - f6;
  }
  double dfdx = (f2-f1)/2.;
  double dfdy = (f4-f3)/2.;
  double d2fdx2 = (f1+f2-2.*f0);
  double d2fdy2 = (f3+f4-2.*f0);
  double d2fdxdy = (f5+f8-f7-f6)/4.;
  double temp = f0 + dfdx*dx + dfdy*dy + 0.5*d2fdx2*dx*dx + 0.5*d2fdy2*dy*dy +
	d2fdxdy*dx*dy;
  return temp;
}

Chunk::Chunk(const std::string& _fitsfile,std::ostream* dbgout) : fitsfile(_fitsfile),
    xmin(0),xmax(0),ymin(0),ymax(0),id(-1),issubchunk(false)
{
#ifdef NOFITS
  xmin=1; xmax=2048; ymin=1;ymax=2048;
  maxj = xmax-xmin; maxi = ymax-ymin;
  dataarray = (float*) malloc(((maxi+1)*(maxj+1))*sizeof(float));
  if (!dataarray) myerror("memory allocation error in Chunk copy constructor");
  data = (float **) malloc((maxi+1)*sizeof(float *));
  if (!data) myerror("memory allocation error in Chunk copy constructor");
  data--;
  for(size_t i=0;i<=maxi;i++) {
    data[i+1] = &dataarray[i*(maxj+1)]-1; // Again with the 1-indexed convention
    for(size_t j=0;j<=maxj;j++)
      data[i+1][j+1] = 0.;
  }
  okget = 1;
#else
  data = chunk_get(&id,&xmin,&xmax,&ymin,&ymax,1,fitsfile.c_str());
  if (chunk_control.bound_flag!=0 && dbgout) 
    *dbgout << "Making chunk.  Bound flag = " << chunk_control.bound_flag <<std::endl;
  maxj = xmax-xmin;
  maxi = ymax-ymin;
#endif
  if (!data) myerror("Making chunk");
}

Chunk::Chunk(const std::string& _fitsfile,size_t _xmin,size_t _xmax,size_t _ymin,size_t _ymax,
    std::ostream* dbgout) :
    fitsfile(_fitsfile),xmin(_xmin),xmax(_xmax),ymin(_ymin),ymax(_ymax),
    id(-1), issubchunk(false)
{
#ifdef NOFITS
  maxj = xmax-xmin; maxi = ymax-ymin;
  dataarray = (float*) malloc(((maxi+1)*(maxj+1))*sizeof(float));
  if (!dataarray) myerror("memory allocation error in Chunk copy constructor");
  data = (float **) malloc((maxi+1)*sizeof(float *));
  if (!data) myerror("memory allocation error in Chunk copy constructor");
  data--;
  for(size_t i=0;i<=maxi;i++) {
    data[i+1] = &dataarray[i*(maxj+1)]-1; // Again with the 1-indexed convention
    for(size_t j=0;j<=maxj;j++)
      data[i+1][j+1] = 0.;
  }
  okget = 1;
#else
  data = chunk_get(&id,&xmin,&xmax,&ymin,&ymax,1,fitsfile.c_str());
  if (chunk_control.bound_flag!=0 && dbgout) 
    *dbgout<<"Making chunk.  Bound flag = "<<chunk_control.bound_flag<<std::endl;
  okget = !chunk_control.bound_flag;
  maxj = xmax-xmin;
  maxi = ymax-ymin;
#endif
  if (!data) myerror("Making chunk");
  if (dbgout) {
    if (xmin != int(_xmin)) *dbgout<<"XMin: "<<_xmin<<" -> "<<xmin<<std::endl;
    if (xmax != int(_xmax)) *dbgout<<"XMax: "<<_xmax<<" -> "<<xmax<<std::endl;
    if (ymin != int(_ymin)) *dbgout<<"YMin: "<<_ymin<<" -> "<<ymin<<std::endl;
    if (ymax != int(_ymax)) *dbgout<<"YMax: "<<_ymax<<" -> "<<ymax<<std::endl;
  }
}

Chunk::Chunk(const Chunk& orig, size_t _xmin,size_t _xmax,size_t _ymin,size_t _ymax,
    std::ostream* dbgout) :
    fitsfile(orig.fitsfile),xmin(_xmin),xmax(_xmax),ymin(_ymin),ymax(_ymax),
    id(orig.id), issubchunk(true)
{
#ifdef NOFITS
  maxj = xmax-xmin; maxi = ymax-ymin;
  int deltai = ymin-orig.ymin;
  int deltaj = xmin-orig.xmin;
  dataarray = (float*) malloc(((maxi+1)*(maxj+1))*sizeof(float));
  if (!dataarray) myerror("memory allocation error in Chunk copy constructor");
  data = (float **) malloc((maxi+1)*sizeof(float *));
  if (!data) myerror("memory allocation error in Chunk copy constructor");
  data--;
  for(size_t i=0;i<=maxi;i++) {
    data[i+1] = &dataarray[i*(maxj+1)]-1; // Again with the 1-indexed convention
    for(size_t j=0;j<=maxj;j++)
      data[i+1][j+1] = orig.data[i+1+deltai][j+1+deltaj]
  }
  okget = 1;
#else
  data = chunk_get(&id,&xmin,&xmax,&ymin,&ymax,0,fitsfile.c_str());
  if (chunk_control.bound_flag!=0 && dbgout) 
    *dbgout<<"Making chunk.  Bound flag = "<<chunk_control.bound_flag<<std::endl;
  okget = !chunk_control.bound_flag;
  maxj = xmax-xmin;
  maxi = ymax-ymin;
#endif
  if (!data) myerror("Making chunk");
  if (dbgout) {
    if (xmin != int(_xmin)) *dbgout<<"XMin: "<<_xmin<<" -> "<<xmin<<std::endl;
    if (xmax != int(_xmax)) *dbgout<<"XMax: "<<_xmax<<" -> "<<xmax<<std::endl;
    if (ymin != int(_ymin)) *dbgout<<"YMin: "<<_ymin<<" -> "<<ymin<<std::endl;
    if (ymax != int(_ymax)) *dbgout<<"YMax: "<<_ymax<<" -> "<<ymax<<std::endl;
  }
}

Chunk::Chunk(const Chunk &rhs) : fitsfile(""),xmin(rhs.xmin),xmax(rhs.xmax),
    ymin(rhs.ymin),ymax(rhs.ymax),maxi(rhs.maxi),maxj(rhs.maxj),
    id(-1), issubchunk(false)
// Creates a copy of a chunk.  This can only be used as a scratch copy, unless
// it is explicitly copied back to the original
{
  dataarray = (float*) malloc(((maxi+1)*(maxj+1))*sizeof(float));
  if (!dataarray) myerror("memory allocation error in Chunk copy constructor");
  data = (float **) malloc((maxi+1)*sizeof(float *));
  if (!data) myerror("memory allocation error in Chunk copy constructor");
  data--;
  // Keep Gary's convention of having data be 1-indexed.

  for(size_t i=0;i<=maxi;i++) {
    data[i+1] = &dataarray[i*(maxj+1)]-1; // Again with the 1-indexed convention
    for(size_t j=0;j<=maxj;j++)
      data[i+1][j+1] = rhs.data[i+1][j+1];
  }
}

Chunk& Chunk::operator=(const Chunk& rhs)
{
  if (&rhs == this) return *this;
  assert(maxi==rhs.maxi);
  assert(maxj==rhs.maxj);
  if (id==-1) { // this is a scratch Chunk.  Just copy data.
    for(size_t i=0;i<=maxi;i++) for(size_t j=0;j<=maxj;j++)
      data[i+1][j+1] = rhs.data[i+1][j+1];
  }
  else { // this is a real Chunk.  Use Gary's chunk_fill.
#ifdef NOFITS
    myerror("id is always -1 for NOFITS compiles (op=)");
#else
    chunk_fill(id,rhs.data);
#endif
  }
  return *this;
}

Chunk::~Chunk()
{
  if (id>-1) {
#ifdef NOFITS
    myerror("id is always -1 for NOFITS compiles (~Chunk)");
#else
    if (!issubchunk) 
      if (chunk_done(id)) myerror("Closing chunk");
#endif
  }
  else { // If id == -1, this is a copy, so no fits file to close.
         // Just free the data storage.
    free(data+1);
    free(dataarray);
  }
}

void Chunk::Flush()
{
#ifndef NOFITS
  if (chunk_flush(id)) myerror("Flushing chunk");
#endif
}

