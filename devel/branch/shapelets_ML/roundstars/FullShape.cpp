#include "FullShape.h"
#include "RSParams.h"
#include <cmath>
#include "BinomFact.h"
#include <vector>

using std::complex;
using std::vector;
using tmv::Matrix;
using tmv::Vector;
using std::endl;

template <class T> inline const T& MIN(const T& a, const T& b)
{ return (a<b) ? a : b; }
template <class T> inline T SQR(const T& x) { return x*x; }


void FullShape::Write(std::ostream& fout,size_t maxorder) const
{
  int oldprec = fout.precision(8);
  std::ios_base::fmtflags oldf = fout.setf(std::ios_base::scientific,std::ios_base::floatfield);
  fout << GetOrder() << ' ';
  if (maxorder == 0 || maxorder > itsorder)
    maxorder = itsorder;
  for(size_t n=0;n<=maxorder;n++) for(size_t j=0;j<=n/2;j++) {
    size_t i=n-j;
    if (i==j) fout << Get(i,j).real() << ' ';
    else fout << Get(i,j) << ' ';
  }
  fout << endl;
  fout.precision(oldprec);
  fout.flags(oldf);
}

void FullShape::WriteCSV(std::ostream& fout) const
{
  int oldprec = fout.precision(8);
  std::ios_base::fmtflags oldf = fout.setf(std::ios_base::scientific,std::ios_base::floatfield);

  size_t ntot = itsorder*(itsorder+3)/2 + 1;
  size_t count=0;
  for(size_t n=0;n<=itsorder;n++) 
    for(size_t j=0;j<=n/2;j++) {
      size_t i=n-j;
      if (i==j)
      {
        fout << Get(i,j).real();
        count++;
      }
      else 
      {
        fout << Get(i,j).real();
        count++;
        fout << ',';
        fout << Get(i,j).imag();
        count++;
      }
      if (count < ntot)
        fout << ',';
    }
  //std::cout<<"count = "<<count<<endl;
  fout.precision(oldprec);
  fout.flags(oldf);
}


void MakeDF(Matrix<double>& DF, const Vector<complex<double> >& F00, size_t order,
    size_t shapeorder, double sigma, const Matrix<double>& jacobian);
void ModifyDF(Matrix<double>& DF, size_t order,vector<size_t> *minimindex);
void Minimize(size_t order, const Vector<double>& K0, const Matrix<double>& A,
    Vector<double> *Y,double sigma);
double Norm(const Kernel& k);
void DefineDPlusMinus(size_t order,double sigma, const Matrix<double>& jacobian,
    Matrix<complex<double> >& DPlus, Matrix<complex<double> >& DPlus2, 
    Matrix<complex<double> >& DPlusAlt, Matrix<complex<double> >& DPlusMinus, 
    Matrix<complex<double> >& DPlusMinusAlt);

void FullShape::FindAnalyticKernel(Kernel *kernelout) const
// Let F be the original star.  F = Sum_pq( A_pq psi_pq )
// where psi_pq are the Harmonic oscillator eigenfunctions
// Then we want to find the kernel K which makes the star round.
// Let G be the convolved star.  G = Sum_pq( C_pq psi_pq )
// And G = K o F = Sum_ijpq ( A_pq K_ij (D+)^i (D-)^j psi_pq )
// Or, G = Sum_ij ( K_ij (D+)^i (D-)^j F )
//
// F, G and K can be thought of as vectors where ij or pq are considered
// a single index.  (The order is 00 10 01 20 11 02 30 21 12 03 etc.)
// Then we essentially have a matrix equation:
// [D_ij F_pq] K_ij = C_pq
// The matrix [D_ij F_pq] has for column ij the vector (D+)^i (D-)^j F.
// This matrix is called DF below and is made by the helper function
// MakeDF().
//
// If we could specify all values of C_pq, we'd be done.  Just solve this
// matrix equation for K_ij.
//
// However, that is not the case.
// C_00 is always arbitrary.  And K_00 is always = 1.
// So the C_00 eqn is changed to be just K_00 = 1.
//
// There are similar other modifications which depend on a parameter
// call roundnesslevel.  These are all made in the helper function
// ModifyDF().  See the comments for that function for more details.
//
{
  dbg<<"start find analytic kernel\n";
  size_t order=kernelout->GetOrder();
  size_t size = KSIZE(order);
  xdbg<<"order = "<<order<<", size = "<<size<<endl;

  Vector<double> C(2*size,0.); 
  Assert(size > 0);
  // The only C_ij that should be non-zero is C00 = 1.
  C(0) = 1.;

  Vector<complex<double> > F00(FS2SIZE(itsorder));
  for(size_t n=0;n<=itsorder;n++) for(int i=n,j=0;i>=j;i--,j++) {
    size_t ij = FS2IJ(i,j);
    Assert(ij < F00.size());
    F00(ij) = Get(i,j);
    if(i!=j) {
      size_t ji = FS2IJ(j,i);
      Assert(ji < F00.size());
      F00(ji) = conj(Get(i,j));
    }
  }
  xxdbg<<"made F00 = "<<F00<<endl;

  Matrix<double> DF(2*size,2*size,0.);
  xxdbg<<"zero DF = "<<DF<<endl;
  MakeDF(DF,F00,order,itsorder,itssigma,itsjacobian);
  xxdbg<<"after make DF = "<<DF<<endl;
  vector<size_t> minimij;
  ModifyDF(DF,order,&minimij);
  xxdbg<<"after modify DF = "<<DF<<endl;
  DF.DivideUsing(tmv::SV);
  DF.SaveDiv();
  Vector<double> K = C/DF;
  xxdbg<<"found K\n";
// This solution is without any minimization parameters.
// The general solution now is K = K0 + A Y
// where K0 is the above solution, A is a matrix whose columns are the
// degeneracy direction vectors, and Y is an arbitrary vector.
// There are as many free parameters (y_i) as there are kernel elements
// we are allowing to vary in order to minimize the norm.
  if (params.dominimization) {
    size_t nparams = minimij.size();
    Vector<double> Y(2*nparams);
    Matrix<double> zero(2*size,2*nparams,0.);
    for(size_t m=0;m<nparams;m++) {
      Assert(minimij[m]<size);
      zero(2*minimij[m],2*m) = 1.;
      zero(2*minimij[m]+1,2*m+1) = 1.;
    }
    Matrix<double> A(2*size,2*nparams);
    xxdbg<<"before find A\n";
    xxdbg<<"zero = "<<zero<<endl;
    xxdbg<<"DF = "<<DF<<endl;
    A = zero/DF;
    xxdbg<<"found A\n";
    Minimize(order,K,A,&Y,itssigma);
    xxdbg<<"after minimize\n";
    K += A*Y;
    xxdbg<<"done dominim\n";
  }

  dbg<<"K = "<<K<<endl;
  *kernelout = K;
  dbg<<"norm of kernel = "<<Norm(*kernelout)<<endl;
}

void SetDFCol(Matrix<double>& DF, size_t colnum, size_t order, const Vector<complex<double> >& F1)
{
  Assert(F1.size() == FS2SIZE(order));
  Assert(2*colnum < DF.rowsize());

  for(size_t j=0;j<=order;j++) for(size_t i=j;i+j<=order;i++) {
    size_t fsij = FS2IJ(i,j); size_t fsji = FS2IJ(j,i); size_t kij = KINDEX(i,j); 
    Assert(fsij < F1.size());
    Assert(fsji < F1.size());
    if (2*kij < DF.colsize()) {
      DF(2*kij,2*colnum) = (F1[fsij].real()+F1[fsji].real())/2.;
      DF(2*kij+1,2*colnum) = (F1[fsij].imag()-F1[fsji].imag())/2.;
      DF(2*kij,2*colnum+1) = (-F1[fsij].imag()-F1[fsji].imag())/2.;
      DF(2*kij+1,2*colnum+1) = (F1[fsij].real()-F1[fsji].real())/2.;
    }
  }
}

void MakeDF(Matrix<double>& DF, const Vector<complex<double> >& F0, size_t order,
    size_t shapeorder, double sigma, const Matrix<double>& jacobian)
// Define the matrix DF.
//
// (Note the similarity to Image::operator*=(const FittedKernel& k) )
//
// One complication is that the D+ and D- (and D+^2, D-^2 and D+D-)
// are not the continuous derivative operators we would like them to be,
// but rather they are discrete convolution masks, which only approximate the
// continuous versions.  Thus DefineDPlusMinus is a helper function
// to make the matrices DPlus, DMinus, DPlus2, DMinus2 and DPlusMinus
// which represent the effect of each of these convolution masks on a
// vector of eigenfunctions.
{
  Assert(F0.size() == FS2SIZE(shapeorder));
  Assert(DF.rowsize() == 2*KSIZE(order));

  size_t size = F0.size();
  Matrix<complex<double> > DPlus(size,size,0.);
  Matrix<complex<double> > DPlus2(size,size,0.);
  Matrix<complex<double> > DPlusAlt(size,size,0.);
  Matrix<complex<double> > DPM(size,size,0.);
  Matrix<complex<double> > DPMAlt(size,size,0.);
  DefineDPlusMinus(shapeorder,sigma,jacobian,DPlus,DPlus2,DPlusAlt,DPM,DPMAlt);

  Vector<complex<double> > F1=F0; // F1 = (D+)^n F0
  for(size_t n=0;n<=order;n+=2) {  // Note: n is always even
    if (n>0) {F1 = DPlus2 * F1;}
    if (n==order) {  // Slight speed-up, since don't copy F1 to F3
      SetDFCol(DF,KINDEX(n,0),shapeorder,F1);
    }
    else {
      Vector<complex<double> > F3 = F1; // F3 = (D+)^n+m(D-)^m F1
      for(size_t m=0;m<=(order-n)/2;m++) {
        if (m>0) {F3 = DPM * F3;}
        SetDFCol(DF,KINDEX(n+m,m),shapeorder,F3);
        if (n+2*m+1 <= order) {
          SetDFCol(DF,KINDEX(n+m+1,m),shapeorder,DPlus*F3);
          SetDFCol(DF,KALTINDEX(n+m+1,m),shapeorder,DPlusAlt*F3);
        }
        if (n+2*m+2 <= order) {
          SetDFCol(DF,KALTINDEX(n+m+1,m+1),shapeorder,DPMAlt*F3);
        }
      }
    }
  }
}

void FullShape::operator*=(const Kernel& kernel)
// Takes a shape and applies a kernel to it.  This effectively gives the
// predicted shape if a star with that shape were actually convolved with
// that kernel.
{
  size_t kernelorder = kernel.GetOrder();
  size_t kernelsize = KSIZE(kernelorder);
  size_t shapeorder = itsorder;
  size_t shapesize = KSIZE(itsorder);

  Matrix<double> DF(2*shapesize,2*kernelsize);
  Vector<complex<double> > F00(FS2SIZE(itsorder));
  for(size_t n=0;n<=itsorder;n++) for(int i=n,j=0;i>=j;i--,j++) {
    F00[FS2IJ(i,j)] = Get(i,j);
    if (i!=j) F00[FS2IJ(j,i)] = conj(Get(i,j));
  }
  MakeDF(DF,F00,kernelorder,shapeorder,itssigma,itsjacobian);
  Vector<double> kvect = kernel.GetDVector();
  Vector<double> NewF = DF * kvect;
  for(size_t q=0;2*q<=shapeorder;q++) {
    Set(q,q,NewF[2*KINDEX(q,q)]);
    for(size_t p=q+1;p+q<=shapeorder;p++) {
      Set(p,q,complex<double> (NewF[2*KINDEX(p,q)],NewF[2*KINDEX(p,q)+1]));
    }
  }
}

void Blank(Matrix<double>& DF, size_t ij)
{
  Assert(2*ij < DF.colsize());
  Assert(2*ij < DF.rowsize());

  DF.row(2*ij).Zero();
  DF.row(2*ij+1).Zero();
  DF(2*ij,2*ij) = 1.;
  DF(2*ij+1,2*ij+1) = 1.;
}

void ModifyDF(Matrix<double>& DF, size_t order, vector<size_t> *minimindex)
// For "level 1" roundness, only C_20 and C_02 are specified to be 0.
// So all K_ij except for K_11, K_20 and K_02 are just set to 0.
// The C_20 and C_02 eqns are as above.  But the C_11 eqn is set to minimize
// the norm of the convolution mask.
// mask = Sum_ij (K_ij D_ij) where D_ij is now the corresponding convolution
// mask, not the idealized operators.
// norm^2 = Sum_mn( (Sum_ij (K_ij (D_ij)_mn))^2 )
// d(norm^2)/dKab = Sum_mn( 2(Sum_ij (K_ij (D_ij)_mn)) (D_ab)_mn) = 0
// Or, Sum_ij( Sum_mn( (D_ab)_mn (D_ij)_mn ) B_ij) = 0
// So this equation is put in for ab = 11 to minimize the norm wrt K11.
//
// For "level 2" roundness, all C_pq with |p-q| = 2 are set to 0.
// And, the norm of the mask is minimized wrt all K_ii (except K_00)
// So the pq eqns with |p-q| = 2 remain.
// And the p=q eqns become the above norm minimization eqns
// All other eqns are set to be K_ij = 0, since we don't have enough constrainst
// to use them.
//
// For "level 3" roundness, all C_pq with p-q = 2 mod 4 are set to 0.
// Otherwise it is identical to level 2.
//
// Finally "level 4" roundness sets to 0 all C_pq with p-q != 0 mod 4.
// All eqns with p - q = 0 mod 4 are changed to the norm minimization eqn.
// 
// Level 5 is the same as level 2, except it uses a different weight
// for the roundness criterion (see IsRound)
//
// Level 6 is allows all the possible kernels to be used for the norm minimization
// and it only uses the 20 component for the roundness criterion (like level 1)
{
  size_t size = KSIZE(order);
  Assert(DF.rowsize() == 2*size);
  Assert(DF.colsize() == 2*size);
  switch (params.roundnesslevel) {
    case 1: // Only 20 and 02 equations
      for(size_t ij=0;ij<size;ij++) {
	if(ij!=KINDEX(2,0)) {
	  Blank(DF,ij);
	}
      }
      minimindex->push_back(KINDEX(1,0));
      minimindex->push_back(KINDEX(1,1));
      minimindex->push_back(KALTINDEX(1,1));
      break;
    case 2: case 5: case 6: // All m = 2 coefficients = 0
      for(size_t j=0;j<=order/2;j++) for(size_t i=j;i+j<=order;i++) {
        if (i-j != 2) {
          size_t ij = KINDEX(i,j);
          Blank(DF,ij);
	  if( ((params.roundnesslevel == 6) || ((i-j)%4==0)) && ij != 0) 
	      minimindex->push_back(ij);
        }
        if (j>0 || i % 2 == 1) {
          size_t altij = KALTINDEX(i,j);
          Blank(DF,altij);
	  if( (params.roundnesslevel == 6) || ((i+j)%2==0) )
	      minimindex->push_back(altij);
        }
      }
      break;
    case 3: // All m = 2 (mod 4) coefficients = 0
      for(size_t j=0;j<=order;j++) for(size_t i=j;i+j<=order;i++) {
        if ((i-j)%4 != 2) {
          size_t ij = KINDEX(i,j);
          Blank(DF,ij);
          if((i-j)%4==0 && ij != 0) minimindex->push_back(ij);
        }
        if (j>0 || i % 2 == 1) {
          size_t altij = KALTINDEX(i,j);
          Blank(DF,altij);
          if ((i+j) % 2 == 0) minimindex->push_back(altij);
        }
      }
      break;
    case 4: // All m != 0 (mod 4) coefficients = 0
      for(size_t j=0;j<=order;j++) for(size_t i=j;i+j<=order;i++) {
        // Only clear the C00 eqn
        if ((i-j)%4==0) {
          size_t ij = KINDEX(i,j);
          Blank(DF,ij);
          if(ij != 0) minimindex->push_back(ij);
        }
        if (j>0 || i % 2 == 1) {
          size_t altij = KALTINDEX(i,j);
          Blank(DF,altij);
          if ((i+j) % 2 == 0) minimindex->push_back(altij);
        }
      }
      break;
    default:
      myerror("Invalid roundness level in FindKernel");
      break;
  }
}

bool FullShape::IsRound(size_t order,double toler) const
// Returns whether the star is sufficiently close to being round.  This is
// based on the roundnesstol parameter, and the roundnesslevel, since
// different roundnesslevels require different coefficients to be 0.
// It only checks the shape up to order (which must be <= itsorder)
{
  static bool first5=(params.roundnesslevel==5);
  static vector<double> weight;
  if (first5) {
    first5=false;
    // This weight corresponds to how much relative effect a residual
    // in the psf can affect the galaxy shape
    // C^00(N+2)N_20 = 2sqrt(pi) sqrt((N+2)(N+1)/2) (1-r)r^N
    // so we weight each residual by (N+2)(N+1)/2 r^2N
    // This is C^2 since we calculate the sum of the squares (norms) below
    // r varies for each galaxy, but we use a maybe representative 
    // value of 0.4 here.
    weight = vector<double>(itsorder/2);
    const double r = 0.5; // some estimate of the mean r of the galaxies.
    const double rsq = r*r;
    double rsqtothei = 1.;
    for(size_t i=0;i<itsorder/2;i++) {
      weight[i] = (i+1)*(i+2)/2. * rsqtothei;
      rsqtothei *= rsq;
    }
    // The values for the first few weights
    //	0	1	2	3	4	5
    //	1	0.75	0.38	0.16	0.06	0.02
  }
 
  if (order==0) order=itsorder;
  if (toler==0.) toler = params.roundnesstol;
  Assert(order <= itsorder);
  double sumsq = 0.;
  switch (params.roundnesslevel) {
    case 1 : case 6 : // just 20 has to be close to 0.  (a02 = Conjugate of a20)
      sumsq = norm(itscoeffs[FSIJ(2,0)]);
      break;
    case 2 : // All i,i-2 need to be 0.
      for(size_t i=2;2*i-2<=order;i++)
        sumsq += norm(itscoeffs[FSIJ(i,i-2)]);
      break;
    case 3 : case 4: // All i,i-2, a_i,i-6, etc.
      for(size_t i=2;i<=order;i++) for(int j=i-2;j>=0;j-=4) if(i+j<=order) {
        sumsq += norm(itscoeffs[FSIJ(i,i-2)]);
      }
      break;
      for(size_t i=2;i<=order;i++) for(int j=i;j>=0;j--) 
       if ((i+j <= order) && (i%4 != size_t(j%4))) {
        sumsq += norm(itscoeffs[FSIJ(i,j)]);
      }
      break;
    case 5 : // Same as 2, but us weight described above
      for(size_t i=2;2*i-2<=order;i++)
        sumsq += norm(itscoeffs[FSIJ(i,i-2)])*weight[i-2];
      break;
    default :
      myerror("Invalid roundnesslevel");
  }
  sumsq /= norm(itscoeffs[0]);
  dbg<<"In IsRound: sumsq = "<<sumsq<<"  toler = "<<toler<<endl;
  return (sumsq < toler);
}

void DefineDPlusMinus(size_t order,double sigma, const Matrix<double>& jacobian,
    Matrix<complex<double> >& MPlus, Matrix<complex<double> >& MPlus2, 
    Matrix<complex<double> >& MPlusAlt, Matrix<complex<double> >& MPlusMinus, 
    Matrix<complex<double> >& MPlusMinusAlt)
{
  // This function is a helper to LinearFindKernel().
  // It just defines the matrices for D+, D-, D+^2, D-^2 and D+D-.
  //
  // If these were continuous derivatives, it would be easy:
  // D+ = (a_g - at_d)/sigma    and   D- = (a_d - at_g)/sigma
  //
  // However, we don't have this.  We have discrete convolutions which are
  // essentiallly the sums of the original function offset by various amounts
  // in different directions.
  // eg. M+ F(x,y) = F(x+dx,y) - F(x-dx,y) + I F(x,y+dy) - I F(x,y-dy)
  // (I use M+ for the discrete mask to differentiate it from the continuous
  // derivative D+.)
  // dx and dy here are 1 pixel, which in dimensionless form is 1/sigma.
  // If we had really small pixels, we probably wouldn't have to worry about
  // this, but with good seeing, we can have FWHM as low as 2.3 pixels, which
  // means sigma is about 1, so dx and dy are each about 1.
  // Therefore, they don't get all that small for high orders.  :(
  //
  // One way to figure out what D+ et al do to an arbitray image is to
  // figure out what they to do our eigenfunctions.
  // Thus we need to expand psi_rs(x+dx,y+dy) in these eigenfunctions.
  // It turns out that the formula is:
  //
  // <pq|rs(z+c)> = (-)^(p+q) / sqrt(p!q!r!s!) exp(-|c|^2/4) 
  //      Sum_i=0..min(p,r) Sum_j=0..min(q,s) [i!j! (pCi)(qCj)(rCi)(sCj) 
  //	(-)^(i+j) (c/2)^(q+r-i-j) (c*/2)^(p+s-i-j)]
  //
  // Here, c is (dx + i dy)/sigma
  // pCi is the binomial coefficient, p!/i!(p-i)!
  //
  // Thus, we can determine the effect of a mask M on psi_rs as
  // Sum_mn M_mn psi_rs(x+m,y+n)
  // where M_mn refers to the coefficient in the convolution mask
  // at position m,n.  (0,0 is the center of the mask.)
  //
  // Define c1 = 1, c2 = I, c3 = 1+I, c4 = 1-I
  //
  // <pq|M+|rs> = 1/2(<pq|rs(c1)> - <pq|rs(-c1)>)
  //     + I/2(<pq|rs(c2)> - <pq|rs(-c2)>)
  // <pq|M-|rs> = 1/2(<pq|rs(c1)> - <pq|rs(-c1)>)
  //     - I/2(<pq|rs(c2)> - <pq|rs(-c2)>)
  // <pq|M+2|rs> = (<pq|rs(c1)> + <pq|rs(-c1)>) - (<ps|rs(c2)> + <pq|rs(-c2)>)
  //     + I/2(<pq|rs(c3)> + <pq|rs(-c3)>) - I/2(<pq|rs(c4)> + <pq|rs(c4)>)
  // <pq|M-2|rs> = (<pq|rs(c1)> + <pq|rs(-c1)>) - (<ps|rs(c2)> + <pq|rs(-c2)>)
  //     - I/2(<pq|rs(c3)> + <pq|rs(-c3)>) + I/2(<pq|rs(c4)> + <pq|rs(c4)>)
  // <pq|M+-|rs> = -4 delta(pq==rs)
  //     + (<pq|rs(c1)> + <pq|rs(-c1)>) + (<pq|rs(c2)> + <pq|rs(-c2)>)
  // <pq|M+*|rs> = (1/4+I/4)(<pq|rs(c3)> - <pq|rs(-c3)>)
  //     + (1/4-I/4)(<pq|rs(c4)> - <pq|rs(-c4)>)
  // <pq|M-*|rs> = (1/4-I/4)(<pq|rs(c3)> - <pq|rs(-c3)>)
  //     + (1/4+I/4)(<pq|rs(c4)> - <pq|rs(-c4)>)
  // <pq|M+-*|rs> = -2 delta(pq==rs)
  //     + 1/2(<pq|rs(c3)> + <pq|rs(-c3)> + <pq|rs(c4)> + <ps|rs(-c4)>)
  //
  // Fortunately, there is a simplification we can do to help us out
  // which depends on the parity of x = q+r-p-s
  // <pq|rs(c)> - <pq|rs(-c)> = 0               for even x
  //                          = 2*<pq|rs(c)>    for odd x
  // <pq|rs(c)> + <pq|rs(-c)> = 2*<pq|rs(c)>    for even x
  //                          = 0               for odd x
  //
  // So for odd x:
  // <pq|M+|rs> = <pq|rs(c1)> + I<pq|rs(c2)>
  // <pq|M-|rs> = <pq|rs(c1)> - I(<pq|rs(c2)>
  // <pq|M+2|rs> = 0
  // <pq|M-2|rs> = 0
  // <pq|M+-|rs> = 0
  // <pq|M+*|rs> = (1/2+I/2)<pq|rs(c3)> + (1/2-I/2)<pq|rs(c4)>
  // <pq|M-*|rs> = (1/2-I/2)<pq|rs(c3)> + (1/2+I/2)<pq|rs(c4)>
  // <pq|M+-*|rs> = 0
  //
  // And for even x:
  // <pq|M+|rs> = 0
  // <pq|M-|rs> = 0
  // <pq|M+2|rs> = 2<pq|rs(c1)> - 2<ps|rs(c2)> + I<pq|rs(c3)> - I<pq|rs(c4)>
  // <pq|M-2|rs> = 2<pq|rs(c1)> - 2<ps|rs(c2)> - I<pq|rs(c3)> + I<pq|rs(c4)>
  // <pq|M+-|rs> = -4 delta(pq==rs) + 2<pq|rs(c1)> + 2<pq|rs(c2)>
  // <pq|M+*|rs> = 0
  // <pq|M-*|rs> = 0
  // <pq|M+-*|rs> = -2 delta(pq==rs) + <pq|rs(c3)> + <pq|rs(c4)>
  //
  // If the c1,c2,etc. values were really as listed above, there are actually
  // some further simplifications.  The problem is that these are the values
  // in (x,y) coords.  The eigenvectors, though, are measured in (u,v)
  // coords, which differs by a factor of sigma as well as being sheared
  // by the jacobian of the distortion.  Therefore, the real values are:
  // c1 = dudx/sigma + I dvdx/sigma
  // c2 = dudy/sigma + I dvdy/sigma
  // c3 = c1+c2
  // c4 = c1-c2
  //
  static size_t maxorder = (params.maxkerneliter+1)*params.outputorder;
  static size_t maxsize = FS2SIZE(maxorder);
  Assert(order <= maxorder);
  static vector<Matrix<double>* > coeffpq(maxsize);
  static vector<Matrix<double>* > coeffrs(maxsize);
  static bool first=true;
  if (first) {
    first = false;
    for(size_t q=0;q<=maxorder;q++) for(size_t p=0;p+q<=maxorder;p++) {
      size_t pq = FS2IJ(p,q);
      Assert(pq < maxsize);
      double sqrtfactpq = sqrtfact(p)*sqrtfact(q);
      coeffpq[pq] = new Matrix<double>(p+1,q+1);
      coeffrs[pq] = new Matrix<double>(p+1,q+1);
      for(size_t i=0;i<=p;i++) for(size_t j=0;j<=q;j++) {
	(*coeffpq[pq])(i,j) = sqrtfact(i)*sqrtfact(j)/sqrtfactpq;
	(*coeffpq[pq])(i,j) *= binom(p,i)*binom(q,j);
	(*coeffrs[pq])(i,j) = (*coeffpq[pq])(i,j);
	if ((p+q+i+j)%2 == 1) (*coeffpq[pq])(i,j) *= -1.;
      }
    }
  } // if(first)

  size_t size = FS2SIZE(order);
  Assert(size <= maxsize);
  complex<double>  c1over2(jacobian(0,0)/(2.*sigma),jacobian(1,0)/(2.*sigma));
  complex<double>  c2over2(jacobian(0,1)/(2.*sigma),jacobian(1,1)/(2.*sigma));
  complex<double>  c3over2 = c1over2+c2over2;
  complex<double>  c4over2 = c1over2-c2over2;
  double exp1 = exp(-norm(c1over2));
  double exp2 = exp(-norm(c2over2));
  double exp3 = exp(-norm(c3over2));
  double exp4 = exp(-norm(c4over2));
  Vector<complex<double> > c1over2tothe(order*2+1,1.);
  Vector<complex<double> > c2over2tothe(order*2+1,1.);
  Vector<complex<double> > c3over2tothe(order*2+1,1.);
  Vector<complex<double> > c4over2tothe(order*2+1,1.);
  for(size_t i=1;i<=order*2;i++) {
    c1over2tothe[i] = c1over2tothe[i-1]*c1over2;
    c2over2tothe[i] = c2over2tothe[i-1]*c2over2;
    c3over2tothe[i] = c3over2tothe[i-1]*c3over2;
    c4over2tothe[i] = c4over2tothe[i-1]*c4over2;
  }
  Vector<complex<double> > exp1conjc1over2tothe(order*2+1);
  Vector<complex<double> > exp2conjc2over2tothe(order*2+1);
  Vector<complex<double> > exp3conjc3over2tothe(order*2+1);
  Vector<complex<double> > exp4conjc4over2tothe(order*2+1);
  for(size_t i=0;i<=order*2;i++) {
    exp1conjc1over2tothe[i] = exp1*conj(c1over2tothe[i]);
    exp2conjc2over2tothe[i] = exp2*conj(c2over2tothe[i]);
    exp3conjc3over2tothe[i] = exp3*conj(c3over2tothe[i]);
    exp4conjc4over2tothe[i] = exp4*conj(c4over2tothe[i]);
  }

  Matrix<complex<double> > pterm(order*2+1,order*2+1);
  Matrix<complex<double> > p2term(order*2+1,order*2+1);
  Matrix<complex<double> > pmterm(order*2+1,order*2+1);
  Matrix<complex<double> > paterm(order*2+1,order*2+1);
  Matrix<complex<double> > pmaterm(order*2+1,order*2+1);
  //Matrix<complex<double> > mterm(order*2+1,order*2+1);
  //Matrix<complex<double> > m2term(order*2+1,order*2+1);
  //Matrix<complex<double> > materm(order*2+1,order*2+1);

  const complex<double>  CI(0,1.);
  const complex<double>  C05p05(0.5,0.5);
  const complex<double>  C05m05(0.5,-0.5);

  for(size_t pow1=0;pow1<=2*order;pow1++) {
    for(size_t pow2=0;pow1+pow2<=2*order;pow2++) {
      complex<double>  temp1 = c1over2tothe[pow1]*
	exp1conjc1over2tothe[pow2];
      complex<double>  temp2 = c2over2tothe[pow1]*
	exp2conjc2over2tothe[pow2];
      complex<double>  temp3 = c3over2tothe[pow1]*
	exp3conjc3over2tothe[pow2];
      complex<double>  temp4 = c4over2tothe[pow1]*
	exp4conjc4over2tothe[pow2];
      int x = pow1-pow2;
      if (x%2 == 0) {
	pmterm(pow1,pow2) = 2.*(temp1+temp2);
	p2term(pow1,pow2) = 2.*(temp1-temp2)+CI*(temp3-temp4);
	//m2term(pow1,pow2) = 2.*(temp1-temp2)-CI*(temp3-temp4);
	pmaterm(pow1,pow2) = temp3+temp4;
      } else {
	pterm(pow1,pow2) = temp1 + CI*temp2;
	//mterm(pow1,pow2) = temp1 - CI*temp2;
	paterm(pow1,pow2) = C05p05*temp3 + C05m05*temp4;
	//materm(pow1,pow2) = C05m05*temp3 + C05p05*temp4;
      }
    }
  }

  Assert(MPlus.colsize() == size && MPlus.rowsize() == size);
  Assert(MPlus2.colsize() == size && MPlus.rowsize() == size);
  Assert(MPlusAlt.colsize() == size && MPlus.rowsize() == size);
  Assert(MPlusMinus.colsize() == size && MPlus.rowsize() == size);
  Assert(MPlusMinusAlt.colsize() == size && MPlus.rowsize() == size);

  size_t pq=0;
  for(size_t pplusq=0;pplusq<=order;pplusq++) {
    for(size_t q=0;q<=pplusq;q++) {
      size_t p = pplusq-q;
      Assert(pq == FS2IJ(p,q));
      Assert(pq < size);
      size_t rs=0;
      for(size_t rpluss=0;rpluss<=order;rpluss++) {
	for(size_t s=0;s<=rpluss;s++) {
	  size_t r = rpluss-s;
	  Assert(rs == FS2IJ(r,s));
	  Assert(rs < size);
	  bool xmod2is0 = ( ((q+r-p-s)%2) == 0);

	  complex<double>  sumpm = (rs==pq ? -4. : 0.);
	  complex<double>  sumpma = (rs==pq ? -2. : 0.);
	  complex<double>  sump=0.,sump2=0.,sumpa=0.;

	  size_t minpr = MIN(p,r);
	  size_t minqs = MIN(q,s);

	  const Matrix<double>& coeffpq1 = *coeffpq[pq];
	  const Matrix<double>& coeffrs1 = *coeffrs[rs];

	  size_t pow1a = q+r;
	  size_t pow2a = p+s;

	  if (xmod2is0) {
	    for(size_t i=0;i<=minpr;i++,pow1a--,pow2a--) {
	      size_t pow1=pow1a, pow2=pow2a;
	      for(size_t j=0;j<=minqs;j++,pow1--,pow2--) {
		double coeff = coeffpq1(i,j) * coeffrs1(i,j);
		//Assert(pow1 == q+r-i-j);
		//Assert(pow2 == p+s-i-j);
		sumpm += coeff*pmterm(pow1,pow2);
		sump2 += coeff*p2term(pow1,pow2);
		sumpma += coeff*pmaterm(pow1,pow2);
	      }
	    }
	  } else {
	    for(size_t i=0;i<=minpr;i++,pow1a--,pow2a--) {
	      size_t pow1=pow1a, pow2=pow2a;
	      for(size_t j=0;j<=minqs;j++,pow1--,pow2--) {
		double coeff = coeffpq1(i,j) * coeffrs1(i,j);
		sump += coeff*pterm(pow1,pow2);
		sumpa += coeff*paterm(pow1,pow2);
	      } 
	    } 
	  }

	  if (xmod2is0) {
	    MPlusMinus(pq,rs) = sumpm;
	    MPlus2(pq,rs) = sump2;
	    MPlusMinusAlt(pq,rs) = sumpma;
	  } else {
	    MPlus(pq,rs) = sump;
	    MPlusAlt(pq,rs) = sumpa;
	  }
	  rs++;
	}
      } // for r,s
      pq++;
    }
  } // for p,q
}

void DefineMasks(size_t order, vector<Matrix<complex<double> >* >& masks)
// This function defines the effective masks that are applied to approximate
// D+, (D+)^2, D+D-, etc. when the convolution is performed.
// (D+)^0(D-)^0 is easy.  It is all 0's except for the center which is 1.
// That is, f_out[i][j] = f_in[i][j]
// All first and second order convolutions are preformed by 3x3 masks:
//
//      [  0   +I/2  0   ]           [  0   -I/2  0   ]
// D+ = [ -1/2  0   +1/2 ]      D- = [ -1/2  0   -1/2 ]
//      [  0   -I/2  0   ]           [  0   +I/2  0   ]
//
//          [ -I/2  -1  +I/2 ]            [ +I/2  -1   -I/2 ]
// (D+)^2 = [ +1     0  +1   ]   (D-)^2 = [ +1     0   +1   ]
//          [ +I/2  +1  -I/2 ]            [ -I/2  -1   +I/2 ]
//
//            [ 0   1   0 ]
// (D+)(D-) = [ 1  -4   1 ]
//            [ 0   1   0 ]
//
// There are 3 more independent 3x3 masks which give us an alternate
// version of D+, D- and (D+D-):
//
//       [ -1/4+I/4  0  1/4+I/4 ]         [ -1/4-I/4  0  1/4-I/4 ]
// D+* = [     0     0     0    ]   D-* = [     0     0     0    ]
//       [ -1/4-I/4  0  1/4-I/4 ]         [ -1/4+I/4  0  1/4+I/4 ]
//
//             [ 1/2   0  1/2 ]
// (D+)(D-)* = [  0   -2   0  ]
//             [ 1/2   0  1/2 ]
//
{
  // First delete the existing masks if any:
  for(size_t i=0;i<masks.size();i++) if (masks[i]) delete masks[i];

  size_t masksize = order+3+order%2;
  // This is 2 larger than is needed to allow a border of 0's to make loop
  // below easier.  (ie. don't have to check for out of bounds on m-1,m+1,etc.)
  size_t middle = (masksize-1)/2;

  masks.resize(KSIZE(order),0);

  masks[0] = new Matrix<complex<double> >(masksize,masksize);
  (*masks[0])(middle,middle) = 1.;

  for(size_t n=0;n<=order;n+=2) {
    if (n>0) { // then define (n,0) and (0,n)
      masks[KINDEX(n,0)] = new Matrix<complex<double> >(masksize,masksize);
      const Matrix<complex<double> >& a = *masks[KINDEX(n-2,0)];
      for(size_t u=1;u<masksize-1;u++) for(size_t v=1;v<masksize-1;v++) {
        (*masks[KINDEX(n,0)])(u,v) = 
          a(u,v+1) + a(u,v-1) - a(u+1,v) - a(u-1,v) +
          complex<double> (0,0.5)*(a(u-1,v-1)+a(u+1,v+1)-a(u+1,v-1)-a(u-1,v+1));
      }
    }
    for(size_t m=0;m<=(order-n)/2;m++) {
      if (m>0) { // then define (n+m,m) and (m,n+m)
        masks[KINDEX(n+m,m)] = new Matrix<complex<double> >(masksize,masksize);
        const Matrix<complex<double> >& a = *masks[KINDEX(n+m-1,m-1)];
        for(size_t u=1;u<masksize-1;u++) for(size_t v=1;v<masksize-1;v++)
          (*masks[KINDEX(n+m,m)])(u,v) =
            (a(u-1,v)+a(u+1,v)+a(u,v-1)+a(u,v+1)) - 4.*a(u,v);
      }
      if (n+2*m+1 <= order) { // then define (n+m+1,m),(m,n+m+1), and alts
        masks[KINDEX(n+m+1,m)] = new Matrix<complex<double> >(masksize,masksize);
        masks[KALTINDEX(n+m+1,m)] = new Matrix<complex<double> >(masksize,masksize);
        const Matrix<complex<double> >& a = *masks[KINDEX(n+m,m)];
        for(size_t u=1;u<masksize-1;u++) for(size_t v=1;v<masksize-1;v++) {
          (*masks[KINDEX(n+m+1,m)])(u,v) =
            ( a(u,v-1) - a(u,v+1)
            + complex<double> (0,1.)*(a(u-1,v)-a(u+1,v)))/2.;
          (*masks[KALTINDEX(n+m+1,m)])(u,v) = 
            complex<double> (0.25,0.25)*(a(u-1,v-1)-a(u+1,v+1))
            + complex<double> (0.25,-0.25)*(a(u+1,v-1)-a(u-1,v+1));
        }
      }
      if (n+2*m+2 <= order) { // then define alt (n+m+1,m+1) and (m+1,n+m+1)
        masks[KALTINDEX(n+m+1,m+1)] = new Matrix<complex<double> >(masksize,masksize);
        const Matrix<complex<double> >& a = *masks[KINDEX(n+m,m)];
        for(size_t u=1;u<masksize-1;u++) for(size_t v=1;v<masksize-1;v++)
          (*masks[KALTINDEX(n+m+1,m+1)])(u,v) =
            0.5*(a(u-1,v-1)+a(u+1,v+1)+a(u-1,v+1)+a(u+1,v-1))
            - 2.*a(u,v);
      }
    }
  }
/*
  if (dbgout) {
    dbg<<"Defined masks.\n";
    size_t i=0,j=0;
    for(size_t k=0;k<(order==2?6:15);k++) {
      dbg<<"i,j,k = "<<i<<','<<j<<','<<k<<endl;
      if (k==0) {i=0;j=0;}
      else if (k==1) {i=1;j=0;}
      else if (k==3) {i=2;j=0;}
      else if (k==6) {i=3;j=0;}
      else if (k==10) {i=4;j=0;}
      else {i--;j++;}
      dbg<<"Mask("<<i<<','<<j<<") =   (norm = "<<Norm(*masks[k])<<")\n";
      dbg << *masks[k];
    }
  }
*/

}

void Minimize(size_t order,const Vector<double>& K0, const Matrix<double>& A,
    Vector<double> *Y,double sigma)
// The general solution to DF K = C is given to be K = K0 + A Y
// where Y is an arbitrary vector of length n.  Also, let the length
// of the vector K be m.  A is then m x n.
// We want to find the particular Y which minimizes the weighted 
// norm of the mask:
//
// f(K) = Sum_mn w_mn | Sum_ij (Dij_mn K_ij) |^2 = Sum_mn w_mn |D_mn K|^2
//
// where D_mn is a vector of all the Dij_mn's
//       D_mn K is a vector dot product
//       w_mn is weight of the mn pixel ( = r^2 )
//
// Note: we don't want to include the 00 term in this sum.
// Effectively we want to minimize the _change_ to the image, not
// minimize the total.
//
// df/dY = 0 implies:
// Sum_mn w_mn (D_mn K)(D_mn A)* = 0
//
// Note that that this is really n equations since Y is a vector, and the
// dot product D_mn A is also a vector.
//
// Since we don't want to solve for K's, but rather for Y's, we need to
// simplify this a bit:
//
// Sum_mn w_mn (D_mn (K0 + A Y))(D_mn A)* = 0
// Sum_mn w_mn (D_mn A Y)(D_mn A)* = -Sum_mn w_mn (D_mn K0)(D_mn A)*
//
// This is now a matrix equation for Y: BY=C
{
  static size_t savedorder=0, masksize=0, size=0;
  static vector<vector<Vector<double>* > > D;
  static vector<vector<double> > rsq;
  if (order > savedorder) {
    vector<Matrix<complex<double> >* > masks;
    DefineMasks(order,masks);
    masksize = masks[0]->rowsize();
    rsq = vector<vector<double> >(masksize,vector<double>(masksize));
    size = KSIZE(order);
    Assert(masks.size()==size_t(size));
    for(size_t m=0;m<D.size();m++) for(size_t n=0;n<D[m].size();n++) {
      if (D[m][n]) delete D[m][n];
    }
    D.resize(masksize);
    for(size_t m=1;m<masksize-1;m++) {
      D[m].resize(masksize);
      for(size_t n=1;n<masksize-1;n++) {
	D[m][n] = new Vector<double>(2*size,0.);
      }
    }
    // Note that we start at k=1, not 0.  Don't want to include 00 mask.
    for(size_t k=1;k<size;k++) {
      for(size_t m=1;m<masksize-1;m++) for(size_t n=1;n<masksize-1;n++) {
        (*D[m][n])(2*k) = (*masks[k])(m,n).real();
        (*D[m][n])(2*k+1) = (*masks[k])(m,n).imag();
      }
    }
    for(size_t m=1;m<masksize-1;m++) for(size_t n=1;n<masksize-1;n++)
      rsq[m][n] = SQR(m-masksize)+SQR(n-masksize);
    savedorder = order;
  }

  //dbg<<"Start minim: K0 = "<<K0<<endl;
  size_t nparams = Y->size();
  //dbg<<"nparams = "<<nparams<<endl;
  Assert(A.colsize() == K0.size() && A.rowsize() == Y->size());

  /* Version for complex
  Matrix<complex<double> > B(nparams,nparams,0.);
  Vector<complex<double> > C(nparams,0.);
  for(size_t m=1;m<masksize-1;m++) for(size_t n=1;n<masksize-1;n++) {
    Vector<complex<double> > DA = (*D[m][n]) * A;
    Vector<complex<double> > DAConj = Conjugate(DA);
    complex<double> DK0 = (*D[m][n]) * K0;
    DAConj *= rsq[m][n];
//    DAConj *= exp(rsq[m][n]/sigsq);
    C -= DAConj * DK0;
    B += DAConj ^ DA;
  }
  */
  Matrix<double> B(nparams,nparams,0.);
  Vector<double> C(nparams,0.);
  for(size_t m=1;m<masksize-1;m++) for(size_t n=1;n<masksize-1;n++) {
    Vector<double> DA = (*D[m][n]) * A;
    double DK0 = (*D[m][n]) * K0;
    //Vector<double> wDA = rsq[m][n] * DA;
    C -= DA * DK0;
    B += rsq[m][n]*DA ^ DA;
  }

  B.DivideUsing(tmv::SV);
  B.SaveDiv();
  B.SetDiv();
  B.SVD().Top(nparams/2,dbgout);
//  B.SVD().Top(nparams/2);
//  B.SVD().Thresh(1.e-4,dbgout);
  *Y = C/B;
}

/* Old MinimizationEqn()
{
  for(size_t i=0;i<=order;i++) for(size_t j=0;j<=order-i;j++) {
    size_t ij = FSIJ(i,j);
    for(size_t m=1;m<masksize-1;m++) for(size_t n=1;n<masksize-1;n++)
      DW[m][n] += masks[i][j][m][n]*dkdk[ij];
  }
  Assert(mineqn->size() == FSSIZE(order));
  for(size_t i=0;i<=order;i++) for(size_t j=0;j<=order-i;j++) {
    size_t ij = FSIJ(i,j);
    (*mineqn)[ij] = 0.;
    for(size_t m=1;m<masksize-1;m++) for(size_t n=1;n<masksize-1;n++)
      (*mineqn)[ij] += conj(DW[m][n]) * masks[i][j][m][n];
  }
}
*/

double Norm(const Kernel& kernel)
{
  size_t order = kernel.GetOrder();
  static size_t savedorder=0;
  static vector<Matrix<complex<double> >* > masks;
  if (order > savedorder) {
    DefineMasks(order,masks);
    savedorder = order;
  }
  size_t masksize = masks[0]->rowsize();
  Matrix<double> totmask(masksize,masksize,0.);
  Vector<complex<double> > kvect = kernel.GetCVector();
  for(size_t k=1;k<masks.size();k++) {
    for(size_t i=0;i<masksize;i++) for(size_t j=0;j<masksize;j++)
      totmask(i,j) += real((kvect[k]*(*masks[k]))(i,j));
      // Imag parts cancel.
  }
  //dbg<< "Total mask = \n" << totmask;
  double temp=0.;
  for(size_t m=1;m<masksize-1;m++) for(size_t n=1;n<masksize-1;n++)
    temp += pow(totmask(m,n),2);
  return temp;
}
