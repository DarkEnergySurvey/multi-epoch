//---------------------------------------------------------------------------
#ifndef FullShapeH
#define FullShapeH
//---------------------------------------------------------------------------

#include "Kernel.h"
#include "TMV.h"
#include <iostream>

// fsij 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20
// i    0  1  2  1  3  2  4  3  2  5  4  3  6  5  4  3  7  6  5  4  8
// j    0  0  0  1  0  1  0  1  2  0  1  2  0  1  2  3  0  1  2  3  0

inline size_t FSIJ(size_t i, size_t j)
  { Assert(i>=j); return (i+j+1)*(i+j+1)/4 + j; }
inline size_t FSSIZE(size_t order)
  { return (order+2)*(order+2)/4; }

// fs2ij 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20
// i     0  1  0  2  1  0  3  2  1  0  4  3  2  1  0  5  4  3  2  1  0
// j     0  0  1  0  1  2  0  1  2  3  0  1  2  3  4  0  1  2  3  4  5

inline size_t FS2IJ(size_t i, size_t j)
  { return (i+j)*(i+j+1)/2 + j; }
inline size_t FS2SIZE(size_t order)
  { return (order+1)*(order+2)/2; }

class FullShape { // The name Shape was already taken.  Used by Uber, Ave.

public:

  //FullShape(size_t _order) : itsorder(_order),itscoeffs(FSSIZE(_order),0.),
  //  itssigma(0), itsjacobian(2,2) {}
  //  Changing default value to -9999
  FullShape(size_t _order) : 
      itsorder(_order),itscoeffs(FSSIZE(_order),-9999.0),
      itssigma(-9999.0), itsjacobian(2,2) {}

  size_t size() const 
  { 
    Assert(itscoeffs.size() == FSSIZE(itsorder)); 
    return itscoeffs.size(); 
  }
  size_t GetOrder() const { return itsorder; }
  tmv::Vector<std::complex<double> >& GetCoeffs() { return itscoeffs; }
  std::complex<double> Get(size_t i,size_t j) const 
  { return itscoeffs[FSIJ(i,j)]; }
  std::complex<double>& operator[](size_t ij) { return itscoeffs[ij]; }
  std::complex<double> operator[](size_t ij) const { return itscoeffs[ij]; }
  double GetSigma() const { return itssigma; }
  const tmv::Matrix<double>& GetJacobian() const { return itsjacobian; }

  void Set(size_t i,size_t j,std::complex<double>  z) 
  { itscoeffs[FSIJ(i,j)] = z; }
  void Set(size_t ij,std::complex<double>  z) { itscoeffs[ij] = z; }
  void SetSigma(double sigma) { itssigma = sigma; }
  void SetJacobian(const tmv::Matrix<double>& _jacob) { itsjacobian = _jacob; }
  void SetJacobian(double a,double b,double c,double d)
  {
    itsjacobian(0,0) = a; itsjacobian(0,1) = b;
    itsjacobian(1,0) = c; itsjacobian(1,1) = d; 
  }

  void Write(std::ostream& fout,size_t maxorder=0) const;
  void WriteCSV(std::ostream& fout) const;

  void operator*=(const Kernel& k);
  void FindAnalyticKernel(Kernel* kout) const;
  bool IsRound(size_t order=0,double rt=0) const;

private:

  size_t itsorder;
  tmv::Vector<std::complex<double> > itscoeffs;
  double itssigma;
  tmv::Matrix<double> itsjacobian;
};

inline std::ostream& operator<<(std::ostream& fout,const FullShape& fs)
{ fs.Write(fout); return fout; }

#endif
