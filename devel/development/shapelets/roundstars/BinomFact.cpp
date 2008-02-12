
#include "BinomFact.h"
#include <vector>
#include <cmath>

using std::vector;

double fact(size_t i)
// return i!
{
  static vector<double> f(10);
  static bool first=true;
  if (first) {
    f[0] = f[1] = 1.;
    for(size_t j=2;j<10;j++) f[j] = f[j-1]*double(j);
    first = false;
  }
  if (i>=f.size()) {
    for(size_t j=f.size();j<=i;j++)
      f.push_back(f[j-1]*double(j));
  }
  return f[i];
}

double isqrt(size_t i)
// return sqrt(i)
{
  static vector<double> f(10);
  static bool first=true;
  if (first) {
    f[0] = 0.;
    f[1] = 1.;
    for(size_t j=2;j<10;j++) f[j] = sqrt(double(j));
    first = false;
  }
  if (i>=f.size())
    for(size_t j=f.size();j<=i;j++)
      f.push_back(sqrt(double(j)));
  return f[i];
}

double sqrtfact(size_t i)
// return sqrt(i!)
{
  static vector<double> f(10);
  static bool first=true;
  if (first) {
    f[0] = f[1] = 1.;
    for(size_t j=2;j<10;j++) f[j] = f[j-1]*sqrt(double(j));
    first = false;
  }
  if (i>=f.size())
    for(size_t j=f.size();j<=i;j++)
      f.push_back(f[j-1]*sqrt(double(j)));
  return f[i];
}

double binom(size_t i,size_t j)
// return iCj, i!/(j!(i-j)!)
{
  static vector<vector<double> > f(10);
  static bool first=true;
  if (first) {
    f[0] = vector<double>(1,1.);
    f[1] = vector<double>(2,1.);
    for(size_t i1=2;i1<10;i1++) {
      f[i1] = vector<double>(i1+1);
      f[i1][0] = f[i1][i1] = 1.;
      for(size_t j1=1;j1<i1;j1++) f[i1][j1] = f[i1-1][j1-1] + f[i1-1][j1];
    }
    first = false;
  }
  if (j>i) return 0.;
  if (i>=f.size()) {
    for(size_t i1=f.size();i1<=i;i1++) {
      f.push_back(vector<double>(i1+1,1.));
      for(size_t j1=1;j1<i1;j1++) f[i1][j1] = f[i1-1][j1-1] + f[i1-1][j1];
    }
  }
  return f[i][j];
}

double sqrtbinom(size_t i,size_t j)
// return iCj, i!/(j!(i-j)!)
{
  static vector<vector<double> > f(10);
  static bool first=true;
  if (first) {
    f[0] = vector<double>(1,1.);
    f[1] = vector<double>(2,1.);
    for(size_t i1=2;i1<10;i1++) {
      f[i1] = vector<double>(i1+1);
      f[i1][0] = f[i1][i1] = 1.;
      for(size_t j1=1;j1<i1;j1++) f[i1][j1] = sqrt(binom(i1,j1));
    }
    first = false;
  }
  if (j>i) return 0.;
  if (i>=f.size()) {
    for(size_t i1=f.size();i1<=i;i1++) {
      f.push_back(vector<double>(i1+1,1.));
      for(size_t j1=1;j1<i1;j1++) f[i1][j1] = sqrt(binom(i1,j1));
    }
  }
  return f[i][j];
}


