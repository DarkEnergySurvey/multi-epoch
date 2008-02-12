//---------------------------------------------------------------------------
#ifndef KernelH
#define KernelH
//---------------------------------------------------------------------------

#include "Legendre2D.h"
#include "Bounds.h" // Actually only uses Position in this header file
#include "TMV.h"
#include "dbg.h"

// New version: Don't bother to keep i<j terms

// Index: 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20
// I:     0  1 *1  2  1 *1  3  2 *3 *2  4  3  2 *3 *2  5  4  3 *5 *4 *3
// J:     0  0 *0  0  1 *1  0  1 *0 *1  0  1  2 *1 *2  0  1  2 *0 *1 *2

inline size_t KSIZE(size_t order)
{ return (order+1)*(order+2)/2;}
inline size_t KINDEX(int i,int j)
{
  Assert(i>=j); 
  return (i+j)*(i+j+1)/2 + j; 
}
inline size_t KALTINDEX(int i,int j)
{
  Assert(i>=j); Assert(j>0 || i/2*2==i-1); 
  return (i+j+1)*(i+j+1)/2 + j; 
}

/* Old version:
// Index: 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20
// I:     0  1  0 *1 *0  2  1  0 *1  3  2  1  0 *3 *2 *1 *0  4  3  2  1
// J:     0  0  1 *0 *1  0  1  2 *1  0  1  2  3 *0 *1 *2 *3  0  1  2  3

inline int KSIZE(int order)
  {return (order+1)*(order+1) + ((order%2==0)?0:1);}
inline int KINDEX(int i,int j)
  {return (i+j)*(i+j) + j + ((i+j)%2==0 && (i>0||j>0)?1:0);}
inline int KALTINDEX(int i,int j)
  {return (i+j)*(i+j+1) + j + 1;}
*/

class Kernel {

  public:

    Kernel(size_t order=0) : itsorder(order), itsk(KSIZE(order)) {}

    void operator=(const tmv::Vector<std::complex<double> >& rhs);
    void operator=(const tmv::Vector<double>& rhs);
    void operator=(const Kernel& rhs);
    Kernel(const Kernel& rhs) : itsorder(rhs.itsorder), itsk(rhs.itsk) {}
    ~Kernel() {}

    size_t GetOrder() const {return itsorder;}
    std::complex<double>  GetIndex(size_t k) const
    {
      Assert(k<itsk.size());
      return itsk[k];
    }
    std::complex<double>  Get(int i,int j) const
    {
      if (i < 0) return GetAlt(-i,j);
      else return itsk[Index(i,j)]; 
    }
    std::complex<double>  GetAlt(int i,int j) const
    {
      return itsk[AltIndex(i,j)];
    }
    tmv::Vector<double> GetDVector() const;
    tmv::Vector<std::complex<double> > GetCVector() const;
    const std::vector<std::complex<double> >& Getvector() const 
    { return itsk; }
    void Set(int i,int j,std::complex<double>  value) 
    { Assert(i+j>0); itsk[Index(i,j)] = value; }
    void SetAlt(int i,int j,std::complex<double>  value) 
    { itsk[AltIndex(i,j)] = value; }
    void SetToUnitKernel();
    void Write(std::ostream& fout) const;
    void Read(std::istream& fin);
    void SetIndex(size_t i, const std::complex<double>& k)
    {
      Assert(i<itsk.size());
      itsk[i] = k;
    }
    //  void operator*=(const Kernel& rhs);
    size_t Index(int i,int j) const 
    {
      Assert(i+j <= int(itsorder));
      Assert(KINDEX(i,j) < KINDEX(i+j+1,0));
      Assert(i>=j);
      return KINDEX(i,j); 
    }
    size_t AltIndex(int i,int j) const 
    {
      Assert(i+j <= int(itsorder));
      Assert(KALTINDEX(i,j) < KINDEX(i+j+1,0));
      Assert(KALTINDEX(i,j) > KINDEX(i+j-(i+j)/2,(i+j)/2));
      Assert(i>=j);
      Assert(j>0 || i/2*2 == i-1);
      return KALTINDEX(i,j); 
    }

  private:

    size_t itsorder; // The maximum value of i+j
    std::vector<std::complex<double> > itsk;
};


class FittedKernel {
  // A Kernel is an array of functions.  k[i][j] refers to the (D+)^i(D-)^j term.
  // But this term varies across the field, so k[i][j] is a function object
  // where k[i][j](x,y) returns the i,j coefficient at position x,y.
  // Note that the function is generally polynomial, but the polynomials don't
  // have to be the same order for each i,j.  And actually, they don't all
  // have to be polynomials either.

  public:

    FittedKernel(size_t order=0); // default with ConstantFunctions of all 0's
    // except for 00 which is 1.0
    FittedKernel(const Kernel& k) : itsorder(k.GetOrder()),
    itsf(KSIZE(itsorder)), itschisq(itsf.size(),0.), itsdof(itsf.size(),0)
    {
      for(size_t i=1;i<itsf.size();i++) {
	itsf[i] = new Constant2D<std::complex<double> >(k.GetIndex(i));
      }
    }
    ~FittedKernel();
    size_t GetOrder() const {return itsorder;}
    const Function2D<std::complex<double> >* GetIndex(size_t k) const
    {
      Assert(k<itsf.size()); 
      return itsf[k]; 
    }
    const Function2D<std::complex<double> >* Get(int i,int j) const
    {
      if (i < 0) return GetAlt(-i,j);
      else return itsf[Index(i,j)]; 
    }
    const Function2D<std::complex<double> >* GetAlt(int i,int j) const
    { return itsf[AltIndex(i,j)]; }
    void Write(std::ostream& fout) const;
    void Read(std::istream& fin);
    void SetFCD(size_t k, Function2D<std::complex<double> > *f,double c,int d)
    {
      Assert(k>=1 && k<itsf.size());
      delete itsf[k]; itsf[k] = f;
      itschisq[k] = c; itsdof[k] = d; 
    }
    size_t Index(int i,int j) const 
    {
      Assert(i+j <= int(itsorder));
      Assert(KINDEX(i,j) < KINDEX(i+j+1,0));
      Assert(i>=j);
      return KINDEX(i,j); 
    }
    size_t AltIndex(int i,int j) const 
    {
      Assert(i+j <= int(itsorder));
      Assert(KALTINDEX(i,j) < KINDEX(i+j+1,0));
      Assert(KALTINDEX(i,j) > KINDEX(i+j-(i+j)/2,(i+j)/2));
      Assert(i>=j);
      Assert(j>0 || i/2*2 == i-1);
      return KALTINDEX(i,j); 
    }
    void operator+=(const FittedKernel& rhs);

    Kernel operator()(double x, double y) const { return operator()(Position(x,y)); }
    Kernel operator()(const Position& pos)  const
    {
      Kernel kern(itsorder);
      for(size_t i=1; i<itsf.size(); i++) {
	kern.SetIndex(i,(*itsf[i])(pos));
      }
      return kern;
    }

  protected:

    size_t itsorder; // The maximum value of i+j
    std::vector<Function2D<std::complex<double> >*> itsf;
    std::vector<double> itschisq;
    std::vector<int> itsdof;

  private:

    FittedKernel(const FittedKernel& rhs); // Don't do this.
    FittedKernel& operator=(const FittedKernel& rhs); // Don't do this either.

};

inline std::ostream& operator<<(std::ostream& fout, const FittedKernel& k)
{ k.Write(fout); return fout; }
inline std::istream& operator>>(std::istream& fin, FittedKernel& k)
{ k.Read(fin); return fin; }
inline std::ostream& operator<<(std::ostream& fout, const Kernel& k)
{ k.Write(fout); return fout; }
inline std::istream& operator>>(std::istream& fin, Kernel& k)
{ k.Read(fin); return fin; }

#endif
