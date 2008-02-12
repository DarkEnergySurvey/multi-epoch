//---------------------------------------------------------------------------
#ifndef ChunkH
#define ChunkH

//---------------------------------------------------------------------------

#include <assert.h>
#include <string>
#include <iostream>

class ChunkRow {

public:

  const float& operator[](size_t j) const
    { assert(j<=maxj); return data[i+1][j+1]; }
  float& operator[](size_t j)
    { assert(j<=maxj); return data[i+1][j+1]; }
  friend class Chunk;  // Only Chunk can create a ChunkRow (private below)

private:

  ChunkRow(float **_data,size_t _i,size_t _maxj) :
    data(_data), i(_i), maxj(_maxj) {}

  float **data;
  size_t i,maxj;
};

class Chunk {

public:

  Chunk(const std::string& _fitsfile, std::ostream* dbgout=0);
  Chunk(const std::string& _fitsfile,size_t _xmin,size_t _xmax,size_t _ymin,size_t _ymax,
      std::ostream* dbgout=0);
  Chunk(const Chunk& orig,size_t _xmin,size_t _xmax,size_t _ymin,size_t _ymax,
      std::ostream* dbgout=0); // get subchunk
  Chunk(const Chunk &rhs);
  Chunk& operator=(const Chunk &rhs);
  ~Chunk();

  int GetXMin() const {return xmin;}
  int GetXMax() const {return xmax;}
  int GetYMin() const {return ymin;}
  int GetYMax() const {return ymax;}
  size_t GetMaxI() const {return maxi;}
  size_t GetMaxJ() const {return maxj;}
  const std::string& GetFitsFile() const {return fitsfile;}

  const ChunkRow operator[](size_t i) const
    { assert(i<=maxi); return ChunkRow(data,i,maxj); }
  ChunkRow operator[](size_t i) // Allows changes to data
    { assert(i<=maxi); return ChunkRow(data,i,maxj); }
  float& operator()(size_t i,size_t j) {return data[i+1][j+1];}
  float operator()(size_t i,size_t j) const {return data[i+1][j+1];}
  void Set(size_t i,size_t j,float value) 
    { assert(i<=maxi&&j<=maxj); data[i+1][j+1] = value; }
  float Get(size_t i,size_t j) const
    { assert(i<=maxi&&j<=maxj); return data[i+1][j+1]; }
  double Interpolate(double x, double y) const;
  double QuadInterpolate(double x, double y) const;

  bool OKGet() const {return okget;}
  void Flush();

private:

  std::string fitsfile;
  int xmin,xmax,ymin,ymax;
  size_t maxi,maxj;
  float **data;
  int id;
  bool okget;
  bool issubchunk;
  float *dataarray; // Only needed for scratch copies of Chunks.  Normally
                    // this is kept in Gary's chunk structure
};


#endif
