
#include "TMV_Sym.h"
#include "TMV.h"
#include "TMV_Diag.h"

namespace tmv {

  template <class T> inline const MatrixView<T>* ZMV() 
  { return (const MatrixView<T>*)(0); }

  template <class T> HermSVDiv<T>::HermSVDiv(const GenSymMatrix<T>& A,
      bool StoreU) :
    U(new Matrix<T,ColMajor>(A.size(),A.size())), S(A.size()), 
    det(RealType(T)(0)), calcdet(false)
  {
    LowerTriMatrixViewOf(*U) = A.LowerTri();
    HermSV_Decompose(U->View(),S.View(),StoreU);
    if (!StoreU) { U.reset(0); }

    Thresh(Epsilon<T>());
  }

  template <class T> SymSVDiv<T>::SymSVDiv(const GenSymMatrix<T>& A, 
      bool StoreU, bool StoreV) :
    U(new Matrix<T,ColMajor>(A.size(),A.size())), S(A.size()), V(0), det(T(1))
  {
    TMVAssert(IsComplex(T()));
    LowerTriMatrixViewOf(*U) = A.LowerTri();
    if (StoreV) {
      V.reset(new Matrix<T,ColMajor>(A.size(),A.size()));
      MatrixView<T> VV = V->View();
      SymSV_Decompose(U->View(),S.View(),&VV,det);
    } else {
      SymSV_Decompose(U->View(),S.View(),ZMV<T>(),det);
    }

    if (!StoreU) { U.reset(0); }

    Thresh(Epsilon<T>());
  }

  template <class T> bool SymSVDiv<T>::CheckDecomp(
      const BaseMatrix<T>& m, ostream* fout) const
  {
    const GenSymMatrix<T>* sm = dynamic_cast<const GenSymMatrix<T>*>(&m);
    TMVAssert(sm);
    if (fout) {
      *fout << "M = "<<tmv::Type(*sm)<<"  "<<*sm<<endl;
      *fout << "U = "<<GetU()<<endl;
      *fout << "S = "<<GetS()<<endl;
      *fout << "V = "<<GetV()<<endl;
    }
    Matrix<T> usv = GetU()*DiagMatrixViewOf(GetS())*GetV();
    RealType(T) nm = Norm(usv-*sm);
    nm /= Norm(GetU())*Norm(GetS())*Norm(GetV());
    if (fout) {
      *fout << "USV = "<<usv<<endl;
      *fout << "Norm(M-USV)/Norm(USV) = "<<nm;
      *fout <<"  "<<Condition()<<" * "<<Epsilon<T>()<<endl;
    }
    return nm < Condition()*sm->colsize()*Epsilon<T>();
  }

  template <class T> bool HermSVDiv<T>::CheckDecomp(
      const BaseMatrix<T>& m, ostream* fout) const
  {
    const GenSymMatrix<T>* sm = dynamic_cast<const GenSymMatrix<T>*>(&m);
    TMVAssert(sm);
    if (fout) {
      *fout << "M = "<<tmv::Type(*sm)<<"  "<<*sm<<endl;
      *fout << "U = "<<GetU()<<endl;
      *fout << "S = "<<GetS()<<endl;
      *fout << "V = "<<GetV()<<endl;
    }
    Matrix<T> usv = GetU()*DiagMatrixViewOf(GetS())*GetV();
    RealType(T) nm = Norm(usv-*sm);
    nm /= Norm(GetU())*Norm(GetS())*Norm(GetV());
    if (fout) {
      *fout << "USV = "<<usv<<endl;
      *fout << "Norm(M-USV)/Norm(USV) = "<<nm;
      *fout <<"  "<<Condition()<<" * "<<Epsilon<T>()<<endl;
    }
    return nm < Condition()*sm->colsize()*Epsilon<T>();
  }

  template <class T> T HermSVDiv<T>::Det() const
  {
    if (!calcdet) {
      det = DiagMatrixViewOf(S).Det();
      calcdet = true;
    }
    return det; 
  }

  template <class T> void HermSVDiv<T>::Thresh(RealType(T) toler,
      ostream* debugout) const
  {
    TMVAssert(toler < RealType(T)(1));
    RealType(T) thresh = abs(S(0))*toler;
    for(kmax=S.size(); kmax>0 && abs(S(kmax-1))<=thresh; --kmax);
    if(debugout) {
      (*debugout)<<"S = "<<S<<endl;
      (*debugout)<<"Smax = "<<abs(S(0))<<", thresh = "<<thresh<<endl;
      (*debugout)<<"kmax = "<<kmax<<" (S.size = "<<S.size()<<")"<<endl;
    }
  }

  template <class T> void HermSVDiv<T>::Top(size_t neigen,
      ostream* debugout) const
  {
    TMVAssert(neigen > 0);
    TMVAssert(neigen <= S.size());
    kmax = neigen;
    if(debugout) {
      (*debugout)<<"S = "<<S<<endl;
      (*debugout)<<"kmax = "<<kmax<<" (S.size = "<<S.size()<<")"<<endl;
    }
  }

  template <class T> void SymSVDiv<T>::Thresh(RealType(T) toler,
      ostream* debugout) const
  {
    TMVAssert(toler < RealType(T)(1));
    RealType(T) thresh = S(0)*toler;
    for(kmax=S.size(); kmax>0 && abs(S(kmax-1))<=thresh; --kmax);
    if(debugout) {
      (*debugout)<<"S = "<<S<<endl;
      (*debugout)<<"Smax = "<<S(0)<<", thresh = "<<thresh<<endl;
      (*debugout)<<"kmax = "<<kmax<<" (S.size = "<<S.size()<<")"<<endl;
    }
  }

  template <class T> void SymSVDiv<T>::Top(size_t neigen,
      ostream* debugout) const
  {
    TMVAssert(neigen > 0);
    TMVAssert(neigen <= S.size());
    kmax = neigen;
    if(debugout) {
      (*debugout)<<"S = "<<S<<endl;
      (*debugout)<<"kmax = "<<kmax<<" (S.size = "<<S.size()<<")"<<endl;
    }
  }

#define InstFile "TMV_SymSVDiv.inst"
#include "TMV_Inst.h"
#undef InstFile

} // namespace mv


