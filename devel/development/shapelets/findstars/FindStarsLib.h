#ifndef _find_stars_lib_h
#define _find_stars_lib_h

#include "unistd.h"
#include <string>
using std::string;
#include "Legendre2D.h"
#include "Bounds.h"
#include "PotentialStar.h"
#include "Histogram.h"
#include <algorithm>
#include <fstream>
using std::ifstream;
using std::ofstream;
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#include <functional>
#include <vector>
using std::vector;
#include <sstream>
using std::istringstream;

using std::abs;

template <class T> inline const T& MIN(const T& a, const T& b)
{ return (a < b) ? a : b; }
template <class T> inline const T& MAX(const T& a, const T& b)
{ return (a > b) ? a : b; }

#include "dbg.h"

void ReadCmdLine(int argc, char **argv,string *infile,string *outfile);
vector<PotentialStar*> FindStars(vector<PotentialStar*>& stars);
void FindMinMax(const vector<PotentialStar*>& list, double *min, double *max,
    const Function2D<double>& f = Constant2D<double>(0.));
void OutlierReject(vector<PotentialStar*>& list, 
    double nsigma, double minsigma,
    const Function2D<double>& f = Constant2D<double>(0.));
vector<PotentialStar*> GetPeakList(const vector<PotentialStar*>& objlist,
    double binsize, double minsize, double maxsize,
    size_t startn, int miniter, double magstep, double maxsignifratio,
    bool firstpass,
    const Function2D<double>& f = Constant2D<double>(0.));
void FitStellarSizes(Function2D<double> *f, size_t order, double sigclip,
    const vector<PotentialStar*>& starlist, double *sigma );
void RoughlyFitBrightStars(const vector<PotentialStar*>& objlist,
    Function2D<double> *f,double *sigma);


#endif
