#include "Kernel.h"
#include <sstream>
#include <string>

using std::complex;

template <class T> inline const T& MIN(const T& a, const T& b)
{ return (a<b) ? a : b; }

void Kernel::Write(std::ostream& fout) const
{
  fout << itsorder << ' ';
  for(size_t n=1;n<=itsorder;n++) for(int j=0;j<=int(n)/2;j++) {
    int i = n-j;
    complex<double>  kij=Get(i,j);
    if (norm(kij)>1.e-20) {
      fout << i << ' ' << j << ' ';
      if (i==j) fout << kij.real() << ' ';
      else fout << kij << ' ';
    }
    if (j>0 || i%2==1) {
      kij = GetAlt(i,j);
      if (norm(kij) > 1.e-20) {
        fout << -i << ' ' << j << ' ';
        if (i==j) fout << kij.real() << ' '; 
	else fout << kij << ' ';
      }
    }
  }
  fout << std::endl;
}

void Kernel::SetToUnitKernel()
{
  for(size_t k=1;k<itsk.size();k++) itsk[k] = 0.;
}

const int MAXLINELEN = 2000;

void Kernel::Read(std::istream& fin)
{
  std::string line;
  if (getline(fin,line)) {
    std::istringstream is(line);
    if (!(is >> itsorder)) myerror("reading order");
    size_t size = KSIZE(itsorder);
    if (size != itsk.size()) itsk.resize(size);

    int i,j;
    complex<double>  kij;
    while (is >> i >> j >> kij) {
      size_t k;
      if (i<0) {i=-i; k = AltIndex(i,j); }
      else {k = Index(i,j); }
      if (k==0 && kij != 1.) myerror("kernel(0,0) must = 1");
      itsk[k] = kij;
    }
    if (!is.eof()) myerror("!eof");
  }
}

/*
void Kernel::operator*=(const Kernel& rhs)
// Finds the effective kernel when this is followed by rhs.
// Let K'_ij be components of rhs and K_ij be the components of this
// Then if G = K F and H = K' G, we want kernel from F -> H
// Then G = Sum_ij (K_ij D_ij F)  (where D_ij is short for (D+)^i(D-)^j )
// H = Sum_kl (K'_kl D_kl G) = Sum_ijkl (K_ij K'_kl D_(i+k),(j+l) F)
// Let m = i+k and n=j+l
// Then H = Sum_mn (Sum_i=0..m Sum_j=0..n (K_ij K'_m-i,n-j)) D_mn F)
// So new kernel element mn = Sum_ij (K_ij K'_m-i,n-j)
//
// Note the order of calculation below.  Working from high m,n down means that
// data is only overwritten when it is no longer needed for later calculations.
{
  Assert(rhs.GetOrder() == GetOrder());
  for(int m=GetOrder();m>=0;m--) for(int n=GetOrder()-m;n>=0;n--) {
    complex<double>  sum=0.;
    for(int i=m;i>=0;i--) for(int j=n;j>=0;j--)
      sum += rhs.Get(m-i,n-j)* this->Get(i,j);
    Set(m,n,sum);
  }
}
*/


/*
Kernel::Kernel(const Kernel& rhs) : FittedKernel(rhs.GetOrder())
{
  for(int m=GetOrder();m>=0;m--) for(int n=GetOrder()-m;n>=0;n--)
    Set(m,n,rhs.Get(m,n));
}
*/

void Kernel::operator=(const Kernel& rhs)
{
  Assert(itsorder = rhs.GetOrder());
  if (this != &rhs) itsk = rhs.itsk;
}

void Kernel::operator=(const tmv::Vector<double>& rhs) 
{
  Assert(rhs.size() == 2*KSIZE(itsorder));
  Assert(itsk.size() == KSIZE(itsorder));
  for(size_t k=1;k<itsk.size();k++) 
    itsk[k] = complex<double> (rhs[2*k],rhs[2*k+1]);
}

tmv::Vector<double> Kernel::GetDVector() const
{
  tmv::Vector<double> temp(2*itsk.size());
  temp[0] = 1.;
  temp[1] = 0.;
  for(size_t k=1;k<itsk.size();k++) {
    temp[2*k] = itsk[k].real();
    temp[2*k+1] = itsk[k].imag();
  }
  return temp; 
}

void Kernel::operator=(const tmv::Vector<complex<double> >& rhs) 
{
  Assert(rhs.size() == KSIZE(itsorder));
  for(size_t k=1;k<rhs.size();k++) itsk[k] = rhs[k];
}

tmv::Vector<complex<double> > Kernel::GetCVector() const
{
  tmv::Vector<complex<double> > temp(itsk.size());
  temp[0] = 1.;
  for(size_t k=1;k<itsk.size();k++) temp[k] = itsk[k];
  return temp; 
}

void FittedKernel::Write(std::ostream& fout) const
{
  fout << itsorder << std::endl;
  for(size_t n=1;n<=itsorder;n++) for(int j=0;j<=int(n)/2;j++) {
    int i = n-j; 
    size_t k=Index(i,j);
    size_t altk = (j>0 || i%2 == 1) ? AltIndex(i,j) : 0;
    if (itsf[k]->NonZero()) {
      double redchisq = itsdof[k] > 0 ? itschisq[k]/itsdof[k] : 0.;
      fout << i << ' ' << j << ' ' << redchisq << ' ';
      fout << itsdof[k] << ' ' << *itsf[k];
    }
    if (altk && itsf[altk]->NonZero()) {
      double redchisq = itsdof[altk] > 0 ? itschisq[altk]/itsdof[altk] : 0.;
      fout << -i << ' ' << j << ' ' << redchisq << ' ';
      fout << itsdof[altk] << ' ' << *itsf[altk];
    }
  }
}

void FittedKernel::Read(std::istream& fin)
{
  if (fin >> itsorder) {
    size_t size = KSIZE(itsorder);
    for(size_t i = size; i < itsf.size(); i++) delete itsf[i];
    itsf.resize(size);
    itschisq.resize(size);
    itsdof.resize(size);

    itsf[0] = new Constant2D<complex<double> >(1.);

    int i,j;
    size_t k;
    double chisq; int dof;
    while (fin >> i >> j >> chisq >> dof) {
      if (i<0) {i=-i; k = AltIndex(i,j); }
      else {k = Index(i,j); }
      itschisq[k] = chisq;
      itsdof[k] = dof;
      if (itsf[k]) delete itsf[k];
      std::auto_ptr<Function2D<complex<double> > > f;
      fin >> f;
      itsf[k] = f.release();
    }
    for(size_t k=1;k<itsf.size();k++) if(!itsf[k])
      itsf[k] = new Constant2D<complex<double> >(0.);
  }
}

FittedKernel::FittedKernel(size_t _order): itsorder(_order),
    itsf(KSIZE(itsorder)),itschisq(KSIZE(itsorder),0.),
    itsdof(KSIZE(itsorder),0)
{
  itsf[0] = new Constant2D<complex<double> >(1.);
  for(size_t k=1;k<itsf.size();k++) {
    itsf[k] = new Constant2D<complex<double> >(0.);
  }
}

FittedKernel::~FittedKernel()
{
  for(size_t k=0;k<itsf.size();k++) {delete itsf[k]; itsf[k]=0;}
}

void FittedKernel::operator+=(const FittedKernel& rhs)
{
  const size_t minorder = MIN(GetOrder(),rhs.GetOrder());
  if (GetOrder() < rhs.GetOrder()) {
    size_t newsize = KSIZE(rhs.GetOrder());
    itsf.reserve(newsize);
    itschisq.reserve(newsize);
    itsdof.reserve(newsize);
    for(size_t i = itsf.size(); i < newsize; i++) {
      itsf.push_back(rhs.itsf[i]->Copy().release());
      itschisq.push_back(rhs.itschisq[i]);
      itsdof.push_back(rhs.itsdof[i]);
    }
  }
  for(size_t i=1;i<KSIZE(minorder);i++) {
    *(itsf[i]) += *(rhs.itsf[i]);
  }
}

