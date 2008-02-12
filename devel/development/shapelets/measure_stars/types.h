#if !defined (_types_h)
#define _types_h

#include <vector>
using std::vector;

typedef char                    int8;
typedef unsigned char           uint8;
typedef short int               int16;
typedef unsigned short int      uint16;
typedef int                     int32;
typedef unsigned int            uint32;
typedef float                   float32;
typedef double                  float64;
#ifdef _WIN32
typedef __int64                 int64;
typedef unsigned __int64        uint64;
#else
typedef long long               int64;
typedef unsigned long long      uint64;
#endif

typedef struct {

  vector<long> id;

  vector<long> i1;
  vector<long> i2;

  vector<long> j1;
  vector<long> j2; 

  vector<float> x0;
  vector<float> y0;
  
  vector<float> x2;
  vector<float> y2;
  vector<float> xy;

  //vector<float> a;

  //vector<float> ellip;

  vector<float> mag_auto;
  vector<float> magerr_auto;

  vector<float> local_sky;
  vector<float> local_skyvar;

  vector<short> flags;

  // Measurements
  vector<float> sh_sigma1; // sigma single pass, low order
  vector<float> sh_sigma;  // final sigma, higher order, only set for stars.
  vector<short> sh_flags;    // processing flags 

  
} cat_struct;

#endif /* _types_h */
