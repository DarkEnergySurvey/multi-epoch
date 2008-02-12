#ifndef _MeasureStars_hpp
#define _MeasureStars_hpp

//#include <boost/random.hpp>

#include "Star.h"
#include "Kernel.h"
#include "RSParams.h"
#include "FullShape.h"

#include "ConfigFile.h"

#include "StarFinder.h"

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>


#include "types.h"
#include "fitsio.h"

#include "Errors.hpp"

using std::string;
using std::ifstream;
using std::ofstream;
using std::stringstream;
using std::endl;
using std::cout;
using std::cerr;
using std::vector;

// MeasureShape flags
#define SH_NOMEAS 0x1
#define SH_FIRSTPASS_FAILED 0x2
#define SH_STAR 0x3
#define SH_SECONDPASS_FAILED 0x4

class MeasureStars
{
public: 
  MeasureStars();
  // Inputs are fits filename and starfinder config file name
  //MeasureStars(string config_file); // Construcor
  MeasureStars(
          string msconf,
          string fsconf, 
          string imfile,
          string catfile,
          string allout,
          string starout);

  ~MeasureStars(); // destructor

  void ReadConfig(string file);
  void ReadFromHeaders(string file);
  void ReadCat();
  void FixCat();
  void ReadSeg(string file);
  void ReadImage(string file, int hdu);
  void ReadFitsCol(fitsfile* fptr, char* colname, int coltype, char* dptr);
  void ResizeCat(long nrows);

  void SetParameters();

  std::vector<string> RequiredConfigFields();

  void SetupNRand();
  void FixSky(int xmin, int xmax, int ymin, int ymax, int id, float sigsky);

  void TestCat();
  void TestImage();
  void FindStars();
  void ProcessCat(int pass);


protected:

  ConfigFile mConfig;

  vector<int16> mSegImage;
  Image mObjImage;
  cat_struct mCat;
  int64 mNRows;

  bool mUseGlobalSky;
  float32 mGlobalSky;
  bool mUseGlobalSkyVar;
  float32 mGlobalSkyVar;

  vector<long> mStarList;

  // Unused currently
  float32 mThreshold;

};

#endif // _MeasureStars_hpp
