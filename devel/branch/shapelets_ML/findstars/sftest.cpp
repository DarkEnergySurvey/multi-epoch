#include "StarFinder.h"
#include "ConfigFile.hpp"
#include <iostream>

std::ostream* dbgout = 0;
bool XDEBUG = false;


int main(int argc, char argv[])
{
  StarFinder sf("findstars.params.default");

  vector<long> id;
  vector<short> flags;
  vector<float> x;
  vector<float> y;
  vector<float> x2;
  vector<float> y2;
  vector<float> mag;

  int n=10;
  id.resize(n);
  flags.resize(n);
  x.resize(n);
  y.resize(n);
  x2.resize(n);
  y2.resize(n);
  mag.resize(n);

  vector<long> starids = sf.ProcessSEList(id,flags,x,y,x2,y2,mag);
  std::cout<<starids[0]<<std::endl;
}
