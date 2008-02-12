#include "Image.h"
#include <fitsio.h>

using std::complex;
using std::vector;
using std::endl;
using tmv::Matrix;
using tmv::MatrixView;
using tmv::Vector;
using tmv::VectorView;
using tmv::ColMajor;
using tmv::VIt;
using tmv::CVIt;
using tmv::Unit;
using tmv::NonConj;

using std::cout;
using std::endl;

template <class T> inline T SQR(const T& x) { return x*x; }
template <class T> inline void SWAP(T& a, T& b) { T temp = a; a = b; b = temp; }

const size_t DIRECTMULTSIZE = 4;

Image::Image() {}
Image::Image(const std::string& filename, int ext) 
{
  LoadFile(filename, ext);
}

void
Image::LoadFile(const std::string& filename, int ext) 
{
  xxdbg<<"Start read fitsimage"<<endl;
  fitsfile *fptr;
  int fitserr=0;

  fits_open_file(&fptr,filename.c_str(),READONLY,&fitserr);
  xxdbg<<"Done open"<<endl;
  if (!fitserr==0) myerror("opening fits file");

  // Move to input extension if not first (1 in cfitsio)
  if (ext > 1)
    {
      int hdutype;
      cout << "Moving to HDU: " << ext << endl;
      fits_movabs_hdu(fptr, ext, &hdutype, &fitserr);
      if (!fitserr==0)
	{
	  fits_report_error(stderr, fitserr); /* print error report */
	  myerror("Moving extension");
	}
      
      if (hdutype != IMAGE_HDU)
	  myerror("Extension is not an IMAGE_HDU");
    }

  int bitpix, naxes;
  long sizes[2];

  fits_get_img_param(fptr, int(2), &bitpix, &naxes, sizes, &fitserr);
  if (!fitserr==0) myerror("reading bitpix, dim, size");

  xxdbg<<"done getimgparam"<<endl;

  if (bitpix != FLOAT_IMG) myerror("image is not float");
  if (naxes != 2) myerror("image is not 2d");
  xxdbg<<"sizes = "<<sizes[0]<<"  "<<sizes[1]<<endl;

  xmin = 0;
  xmax = sizes[0];
  ymin = 0;
  ymax = sizes[1];
  sourcem = new Matrix<double,ColMajor>(xmax,ymax);
  xxdbg<<"done make matrix of image"<<endl;
 
  long fpixel[2] = {1,1};
  int anynul;
  xxdbg<<"Before read_pix\n";
  fits_read_pix(fptr,TDOUBLE,fpixel,long(xmax*ymax),0,sourcem->ptr(),&anynul,
      &fitserr);
  xxdbg<<"done readpix  "<<fitserr<<endl;
  if (!fitserr==0) myerror("reading image data");

  itsm = new MatrixView<double>(sourcem->View());
  xxdbg<<"Done make matrixview"<<endl;

  fits_close_file(fptr, &fitserr);
  if (!fitserr==0) myerror("closing fits file");
}




void Image::Flush(const std::string& filename)
{
  fitsfile *fptr;
  int fitserr=0;
  fits_open_file(&fptr,filename.c_str(),READWRITE,&fitserr);
  if (!fitserr==0) myerror("opening fits file");

  long fpixel[2] = {1,1};
  fits_write_pix(fptr,TDOUBLE,fpixel,long(xmax*ymax),itsm->ptr(),&fitserr);
  if (!fitserr==0) myerror("writing image data");

  fits_close_file(fptr, &fitserr);
  if (!fitserr==0) myerror("closing fits file");
}

vector<vector<Image*> > Image::Divide(size_t nx, size_t ny) const
{
  vector<size_t> x(nx+1);
  vector<size_t> y(ny+1);
  x[0] = xmin;  x[nx] = xmax;
  y[0] = ymin;  y[ny] = ymax;
  size_t xstep = (xmax-xmin)/nx;
  size_t ystep = (ymax-ymin)/ny;
  for(size_t i=1;i<nx;i++) x[i] = x[i-1]+xstep;
  for(size_t j=1;j<ny;j++) y[j] = y[j-1]+ystep;
  vector<vector<Image*> > blockimages(nx,vector<Image*>(ny));
  for(size_t i=0;i<nx;i++) for(size_t j=0;j<ny;j++) 
    blockimages[i][j] = new Image(itsm->SubMatrix(x[i],x[i+1],y[j],y[j+1]),
	  x[i],x[i+1],y[j],y[j+1]);
  return blockimages;
}

double Image::Interpolate(double x, double y) const
{
  Assert(x>=double(xmin) && x<double(xmax));
  Assert(y>=double(ymin) && y<double(ymax));
  int i = int(floor(x-0.5));
  double dx = x - (i+0.5);
  int j = int(floor(y-0.5));
  double dy = y - (j+0.5);
  Assert(dx >= 0. && dx < 1.);
  Assert(dy >= 0. && dy < 1.);

/* 
   2    3
     x      the point (x,y) is within square of points 0,1,2,3 as shown
    
   0    1 

  Since the points are really the values at the center of the pixel, it
  is possible for x to fall closer to the edge of the chip than any points.
  In this case, the function does an extrapolation given the nearest
  square of pixels.
*/
  if (i==int(xmax-1)) {i--; dx += 1.;}
  if (j==int(ymax-1)) {j++; dy += 1.;}
  if (i==-1) {i++; dx -= 1.;}
  if (j==-1) {j++; dy -= 1.;}
  Assert(i>=0 && j>=0 && i+1<int(itsm->colsize()) && j+1<=int(itsm->rowsize()));

  double f0 = (*itsm)(i,j);
  double f1 = (*itsm)(i+1,j);
  double f2 = (*itsm)(i+1,j);
  double f3 = (*itsm)(i+1,j+1);
  double dfdx = f1-f0;
  double dfdy = f2-f0;
  double d2fdxdy = f3+f0-f1-f2;
  return f0 + dfdx*dx + dfdy*dy + d2fdxdy*dx*dy;
}
  
double Image::QuadInterpolate(double x, double y) const
{
  static size_t count=0;
  ++count;
  Assert(x>=xmin && x< xmax);
  Assert(y>=ymin && y< ymax);
  size_t i = size_t (floor(x));
  double dx = x - (i+0.5);
  size_t j = size_t (floor(y));
  double dy = y - (j+0.5);
  Assert(i<itsm->colsize());
  Assert(j<itsm->rowsize());
  Assert (fabs(dx) <= 0.5);
  Assert (fabs(dy) <= 0.5);

/*
  7   4   8

       x          (x,y) is closer to point 0 than any other.
  1   0   2 


  5   3   6

  If any points are off the edge, we set them to the value they would
  have if the second derivative were 0 there.
*/
  double f0 = (*itsm)(i,j);
  double f1 = (i > 0) ? (*itsm)(i-1,j) : 0.;
  double f2 = (i < itsm->colsize()-1) ? (*itsm)(i+1,j) : 0.;
  double f3 = (j > 0) ? (*itsm)(i,j-1) : 0.;
  double f4 = (j < itsm->rowsize()-1) ? (*itsm)(i,j+1) : 0.;
  double f5 = (i > 0 && j > 0) ? (*itsm)(i-1,j-1) : 0.;
  double f6 = (i < itsm->colsize()-1 && j > 0) ? (*itsm)(i+1,j-1) : 0.;
  double f7 = (i > 0 && j < itsm->rowsize()-1) ? (*itsm)(i-1,j+1) : 0.;
  double f8 = (i < itsm->colsize()-1 && j < itsm->rowsize()-1) ?
    (*itsm)(i+1,j+1) : 0.;
  if (i == 0) {
    f1 = 2*f0 - f2;
    f5 = 2*f3 - f6;
    f7 = 2*f4 - f8;
  }
  if (i == itsm->colsize()-1) {
    f2 = 2*f0 - f1;
    f6 = 2*f3 - f5;
    f8 = 2*f4 - f7;
  }
  if (j == 0) {
    f3 = 2*f0 - f4;
    f5 = 2*f1 - f7;
    f6 = 2*f2 - f8;
  }
  if (j == itsm->rowsize()-1) {
    f4 = 2*f0 - f3;
    f7 = 2*f1 - f5;
    f8 = 2*f2 - f6;
  }
  double dfdx = (f2-f1)/2.;
  double dfdy = (f4-f3)/2.;
  double d2fdx2 = (f1+f2-2.*f0);
  double d2fdy2 = (f3+f4-2.*f0);
  double d2fdxdy = (f5+f8-f7-f6)/4.;
  double temp = f0 + dfdx*dx + dfdy*dy + 0.5*d2fdx2*dx*dx + 0.5*d2fdy2*dy*dy +
	d2fdxdy*dx*dy;
  return temp;
}

class CImage {

  public:

    CImage() {}

    CImage(const Image& rhs) : 
      itsrealm(new tmv::Matrix<double,tmv::ColMajor>(rhs.GetM())),
      itsimagm(new tmv::Matrix<double,tmv::ColMajor>(
	    itsrealm->colsize(),itsrealm->rowsize())),
      x0(rhs.GetXMin()+0.5), y0(rhs.GetYMin()+0.5),iscomplex(false) {}

    CImage(const CImage& rhs) : 
      itsrealm(new tmv::Matrix<double,tmv::ColMajor>(rhs.GetRealM())),
      itsimagm(rhs.iscomplex ? 
	  new tmv::Matrix<double,tmv::ColMajor>(rhs.GetImagM()) :
	  new tmv::Matrix<double,tmv::ColMajor>(
	    itsrealm->colsize(),itsrealm->rowsize())),
      x0(rhs.x0), y0(rhs.y0),iscomplex(rhs.iscomplex) {}

    ~CImage() { delete itsrealm; delete itsimagm; }

    CImage& operator=(const CImage& rhs) 
    {
      *itsrealm = rhs.GetRealM();
      if (rhs.iscomplex) {
	*itsimagm = rhs.GetImagM();
	iscomplex = true;
      } else {
	iscomplex = false;
      }
      x0 = rhs.x0;
      y0 = rhs.y0;
      return *this;
    }

    const tmv::Matrix<double,tmv::ColMajor>& GetRealM() const 
    { return *itsrealm; }
    const tmv::Matrix<double,tmv::ColMajor>& GetImagM() const 
    { return *itsimagm; }
    bool IsComplex() const { return iscomplex; }
    //std::complex<double>& operator()(size_t i,size_t j) 
    //{ return (*itsm)(i,j); }
    //std::complex<double> operator()(size_t i,size_t j) const 
    //{ return (*itsm)(i,j); }

    void DPlus() { DPM1(true); }
    void DMinus() { DPM1(false); }
    void DPlus2() { DPM2(true); }
    void DMinus2() { DPM2(false); }
    void DPlusMinus() { DPM0(); }
    void DPlusAlt() { DPM1Alt(true); }
    void DMinusAlt() { DPM1Alt(false); }
    void DPlusMinusAlt() { DPM0Alt(); }

    void operator*=(const Function2D<std::complex<double> > &f);

  private:

    void RecursiveMult(size_t xl,size_t xr,size_t yt,size_t yb,
	std::complex<double> ftl, std::complex<double> ftr,
	std::complex<double> fbl, std::complex<double> fbr,
	const Function2D<std::complex<double> >& f, bool& madecomplex);

    void DPM0();
    void DPM1(const bool plus);
    void DPM2(const bool plus);
    void DPM0Alt();
    void DPM1Alt(const bool plus);

    tmv::Matrix<double,tmv::ColMajor>* itsrealm;
    tmv::Matrix<double,tmv::ColMajor>* itsimagm;
    double x0,y0;
    bool iscomplex;
};

void Image::operator=(const CImage& rhs) 
{ *itsm = rhs.GetRealM(); }

void Image::operator+=(const CImage& rhs) 
{ *itsm += rhs.GetRealM(); }

void Image::operator*=(const FittedKernel &k)
{
  xdbg<<"Start image *= k\n";

  size_t order = k.GetOrder();

  CImage im1 = *this; // im1 is always (D+)^n im, with n even.

  for(size_t n=0;n<=order;n+=2) {
    if (n>0) im1.DPlus2();
    CImage im2 = im1; // im2 is always (D+)^n+m (D-)^m im
    for(size_t m=0;m<=(order-n)/2;m++) {
      if (m>0) im2.DPlusMinus();
      // We have 4 kernel elements to do:
      // (n+m,m) + (0,0), (1,0), (1,0)* and (1,1)*
      // where * indicates the alternate version.
      for(int i=3;i>=0;i--) {
	xxdbg<<"nmi = "<<n<<" "<<m<<" "<<i<<endl;
	if (n+2*m == order && i>0) continue; // only do this kernel element
	if (n+2*m+1 == order && i==3) continue; // don't add (1,1)*
	if (n==0 && m==0 && i==0) continue; // skip (0,0)
	const Function2D<complex<double> >* f=0;
	switch(i) {
	  case 0 : f = k.Get(n+m,m); break;
	  case 1 : f = k.Get(n+m+1,m); break;
	  case 2 : f = k.GetAlt(n+m+1,m); break;
	  case 3 : f = k.GetAlt(n+m+1,m+1); break;
	}
	if (i==0 && n+2*m==order) {
	  // then don't need im2 after this, so ok to modify
	  // Faster, since don't need to create im3.
	  im2 *= *f;
	  *this += im2;
	} else {
	  xxdbg<<"before copy\n";
	  CImage im3 = im2;
	  xxdbg<<"before dplus, etc\n";
	  switch (i) {
	    case 0: break; // im3 already ok
	    case 1: im3.DPlus(); break;
	    case 2: im3.DPlusAlt(); break;
	    case 3: im3.DPlusMinusAlt(); break;
	  }
	  xxdbg<<"before *= f\n";
	  im3 *= *f;
	  xxdbg<<"before += im\n";
	  *this += im3;
	  xxdbg<<"after += im\n";
	}
      }
    }
  }
  
  xdbg<<"Done image *= k\n";
}

void CImage::operator*=(const Function2D<complex<double> > &f)
// Note that i is the y coord, j is the x coord
{
  xxdbg<<"start op*= f\n";
  size_t xl = 0;
  size_t xr = itsrealm->colsize();
  size_t yb = 0;
  size_t yt = itsrealm->rowsize();

  xxdbg<<"f = "<<f<<endl;
  if (f.GetXOrder()==0 && f.GetYOrder()==0) {
    xxdbg<<"Direct Multiply, since f is constant\n";
    complex<double> z = f(0.,0.);
    if (imag(z) == 0.) {
      if (iscomplex) *itsimagm *= real(z);
      *itsrealm *= real(z);
    } else {
      if (iscomplex) {
	Matrix<double,ColMajor> origimagm = *itsimagm;
	*itsimagm *= real(z);
	*itsimagm += imag(z)*(*itsrealm);
	*itsrealm *= real(z);
	*itsrealm -= imag(z)*origimagm;
      } else {
	*itsimagm = *itsrealm*imag(z);
	*itsrealm *= real(z);
	iscomplex = true;
      }
    }
  } else {
    xxdbg<<"f(xl,yt) = "<<f(xl,yt)<<endl;
    xxdbg<<"f(xr,yt) = "<<f(xr,yt)<<endl;
    xxdbg<<"f(xl,yb) = "<<f(xl,yb)<<endl;
    xxdbg<<"f(xr,yb) = "<<f(xr,yb)<<endl;
    bool madecomplex=false;
    RecursiveMult(xl,xr,yt,yb,f(xl,yt),f(xr,yt),f(xl,yb),f(xr,yb),f,
	madecomplex);
    if (madecomplex) iscomplex = true;
  }
}

template <class T> inline void MultF(T f, double& x, double& y)
  // (x+iy) *= f  
  // This one is for real f
{ y *= f; x *= f; }

template <class T> inline void MultF2(T f, double& x, double& y)
  // x+iy = f*x
  // This one is for real f
{ x *= f; }

template <class T> inline void MultF(complex<T> f, double& x, double& y)
  // (x+iy) *= f
{
  double origy = y;
  y = y*real(f)+x*imag(f);
  x = x*real(f)-origy*imag(f);
}

template <class T> inline void MultF2(complex<T> f, double& x, double& y)
  // x+iy = f*x
{
  y = x*imag(f);
  x *= real(f);
}

template <class T>
void DoLinearMult(size_t xl, size_t xr, size_t yt, size_t yb,
    T ftl, T ftr, T fbl, T fbr,
    Matrix<double,ColMajor>* realm, Matrix<double,ColMajor>* imagm,
    bool iscomplex)
{
  xxdbg<<"start linear mult\n";
  T dfb = fbr-fbl;
  T dft = ftr-ftl;
  T dfl = ftl-fbl;
  double dx = double(xr)-double(xl);
  double dy = double(yt)-double(yb);

  T dfdx = dfb/dx;
  T dfldy = dfl/dy;
  T d2fdxdy = (dft-dfb)/dx/dy;
  T f0 = fbl+0.5*dfldy;

  for(size_t y=yb;y<yt;y++) {
    T flinear = f0+0.5*dfdx;
    VIt<double,Unit,NonConj> realit = realm->col(y).begin()+xl;
    VIt<double,Unit,NonConj> imagit = imagm->col(y).begin()+xl;
    if (iscomplex) {
      for(size_t x=xl; x<xr; x++,realit++,imagit++,flinear+=dfdx) 
	MultF(flinear,*realit,*imagit);
    } else {
      for(size_t x=xl; x<xr; x++,realit++,imagit++,flinear+=dfdx) 
	MultF2(flinear,*realit,*imagit);
    }
    f0 += dfldy;
    dfdx += d2fdxdy;
  }
  xxdbg<<"done linear mult\n";
}

bool LinearMult(size_t xl, size_t xm, size_t xr, 
    size_t yt, size_t ym, size_t yb,
    complex<double> ftl, complex<double> ft, complex<double> ftr,
    complex<double> fl, complex<double> fm, complex<double> fr,
    complex<double> fbl, complex<double> fb, complex<double> fbr,
    double relthresh,
    Matrix<double,ColMajor>* realm, Matrix<double,ColMajor>* imagm,
    bool iscomplex, bool& madecomplex)
{
  double thresh = fabs(real(fm));
  thresh += fabs(real(fl));
  thresh += fabs(real(fr));
  thresh += fabs(real(ft));
  thresh += fabs(real(fb));
  double imagthresh = fabs(imag(fm));
  imagthresh += fabs(imag(fl));
  imagthresh += fabs(imag(fr));
  imagthresh += fabs(imag(ft));
  imagthresh += fabs(imag(fb));

  if (imagthresh > 0.) thresh += imagthresh;
  thresh *= relthresh;

  double fl_linear = (real(ftl)+real(fbl))/2.;
  double absdiff = fabs(fl_linear-real(fl));
  if (absdiff >= thresh) return false;

  double fr_linear = (real(ftr)+real(fbr))/2.;
  absdiff += fabs(fr_linear-real(fr));
  if (absdiff >= thresh) return false;

  double ft_linear = (real(ftl)+real(ftr))/2.;
  absdiff += fabs(ft_linear-real(ft));
  if (absdiff >= thresh) return false;

  double fb_linear = (real(fbl)+real(fbr))/2.;
  absdiff += fabs(fb_linear-real(fb));
  if (absdiff >= thresh) return false;

  double fm_linear = (real(ftl)+real(ftr)+real(fbl)+real(fbr))/4.;
  absdiff += fabs(fm_linear-real(fm));
  if (absdiff >= thresh) return false;

  if (imagthresh > 0.) {
    double fl_linear = (imag(ftl)+imag(fbl))/2.;
    absdiff += fabs(fl_linear-imag(fl));
    if (absdiff >= thresh) return false;

    double fr_linear = (imag(ftr)+imag(fbr))/2.;
    absdiff += fabs(fr_linear-imag(fr));
    if (absdiff >= thresh) return false;

    double ft_linear = (imag(ftl)+imag(ftr))/2.;
    absdiff += fabs(ft_linear-imag(ft));
    if (absdiff >= thresh) return false;

    double fb_linear = (imag(fbl)+imag(fbr))/2.;
    absdiff += fabs(fb_linear-imag(fb));
    if (absdiff >= thresh) return false;

    double fm_linear = (imag(ftl)+imag(ftr)+imag(fbl)+imag(fbr))/4.;
    absdiff += fabs(fm_linear-imag(fm));
    if (absdiff >= thresh) return false;

    DoLinearMult(xl,xm,ym,yb,fl,fm,fbl,fb,realm,imagm,iscomplex);
    DoLinearMult(xl,xm,yt,ym,ftl,ft,fl,fm,realm,imagm,iscomplex);
    DoLinearMult(xm,xr,ym,yb,fm,fr,fb,fbr,realm,imagm,iscomplex);
    DoLinearMult(xm,xr,yt,ym,ft,ftr,fm,fr,realm,imagm,iscomplex);
    madecomplex = true;
    return true;
  } else { // f is real
    DoLinearMult(xl,xm,ym,yb,real(fl),real(fm),real(fbl),real(fb),
	realm,imagm,iscomplex);
    DoLinearMult(xl,xm,yt,ym,real(ftl),real(ft),real(fl),real(fm),
	realm,imagm,iscomplex);
    DoLinearMult(xm,xr,ym,yb,real(fm),real(fr),real(fb),real(fbr),
	realm,imagm,iscomplex);
    DoLinearMult(xm,xr,yt,ym,real(ft),real(ftr),real(fm),real(fr),
	realm,imagm,iscomplex);
    return true;
  }
}

void CImage::RecursiveMult(size_t xl,size_t xr,size_t yt,size_t yb,
    complex<double> ftl, complex<double> ftr,
    complex<double> fbl, complex<double> fbr,
    const Function2D<complex<double> >& f, bool& madecomplex)
{
  xxdbg<<"Start recurse:\n";
  xxdbg<<"Bottom Left: f("<<xl<<','<<yb<<") = "<<fbl<<endl;
  xxdbg<<"Bottom Right: f("<<xr<<','<<yb<<") = "<<fbr<<endl;
  xxdbg<<"Top Left: f("<<xl<<','<<yt<<") = "<<ftl<<endl;
  xxdbg<<"Top Right: f("<<xr<<','<<yt<<") = "<<ftr<<endl;

  static const double relthresh = 1.e-2; // 1% difference

  if (xr-xl <= DIRECTMULTSIZE || yt-yb <= DIRECTMULTSIZE) { 
    // too small.  just directly mult
    xxdbg<<"Direct\n";
    for(size_t y=yb;y<yt;y++) {
      VIt<double,Unit,NonConj> realit = itsrealm->col(y).begin()+xl;
      VIt<double,Unit,NonConj> imagit = itsimagm->col(y).begin()+xl;
      double yy = y+y0;
      double xx = xl+x0;
      for(size_t x=xl;x<xr;x++,xx+=1.,realit++,imagit++) {
	complex<double> z = f(xx,yy);
	if (imag(z) == 0.) {
	  if (iscomplex) *imagit *= real(z);
	  *realit *= real(z);
	} else {
	  if (iscomplex) {
	    double origimag = *imagit;
	    *imagit = *imagit * real(z) + *realit * imag(z);
	    *realit = *realit * real(z) - origimag * imag(z);
	  } else {
	    *imagit = *realit*imag(z);
	    *realit *= real(z);
	    madecomplex = true;
	  }
	}
      }
    }
  } else {
    xxdbg<<"Not Direct\n";

    size_t xm = (xr+xl)/2;
    size_t ym = (yt+yb)/2;
    xxdbg<<"xm,ym = "<<xm<<"  "<<ym<<endl;
    complex<double> fm = f(xm,ym);
    xxdbg<<"fm = "<<fm<<endl;
    complex<double> fl = f(xl,ym);
    xxdbg<<"fl = "<<fl<<endl;
    complex<double> fr = f(xr,ym);
    xxdbg<<"fr = "<<fr<<endl;
    complex<double> ft = f(xm,yt);
    xxdbg<<"ft = "<<ft<<endl;
    complex<double> fb = f(xm,yb);
    xxdbg<<"fb = "<<fb<<endl;

    // See if linear is a good estimate at all of these:

    if (!LinearMult(xl,xm,xr,yt,ym,yb,ftl,ft,ftr,fl,fm,fr,fbl,fb,fbr,
	  relthresh,itsrealm,itsimagm,iscomplex,madecomplex)) {
      RecursiveMult(xl,xm,yt,ym,ftl,ft,fl,fm,f,madecomplex); // top left quad
      RecursiveMult(xm,xr,yt,ym,ft,ftr,fm,fr,f,madecomplex); // top right
      RecursiveMult(xl,xm,ym,yb,fl,fm,fbl,fb,f,madecomplex); // bottom left
      RecursiveMult(xm,xr,ym,yb,fm,fr,fb,fbr,f,madecomplex); // bottom right
    }
  }
  xxdbg<<"Done recurse\n";
}

void CImage::DPM1(const bool plus)
  // DPlus and DMinus are represented by convolutions with the matrices:
  //   [  0   +I/2  0   ]     [  0   -I/2  0   ]
  //  ^[ -1/2  0   +1/2 ] and [ -1/2  0   +1/2 ] respectively.
  //  y[  0   -I/2  0   ]     [  0   +I/2  0   ]
  //   x->
  //
  // Note: rows go along y direction, so these are actually rotate 90 degrees
  // from how the Matrix stores them.
  //
{
  xxdbg<<"Start dpm1 "<<plus<<endl;
  size_t N = itsrealm->colsize();
  Vector<double>* origprevrealcol = new Vector<double>(itsrealm->col(0));
  Vector<double>* origthisrealcol = new Vector<double>(N);
  Vector<double>* origprevimagcol = iscomplex ? 
    new Vector<double>(itsimagm->col(0)) : 0;
  Vector<double>* origthisimagcol = iscomplex ? 
    new Vector<double>(N) : 0;
  for(size_t i=1;i<itsrealm->rowsize()-1;i++) {
    *origthisrealcol = itsrealm->col(i);
    if (iscomplex) *origthisimagcol = itsimagm->col(i);
    VectorView<double> realcoli = itsrealm->col(i,1,N-1);
    VectorView<double> imagcoli = itsimagm->col(i,1,N-1);
    realcoli = 0.5*origthisrealcol->SubVector(2,N);
    realcoli += -0.5*origthisrealcol->SubVector(0,N-2);
    if (iscomplex) {
      imagcoli = 0.5*origthisimagcol->SubVector(2,N);
      imagcoli += -0.5*origthisimagcol->SubVector(0,N-2);
      if (plus) {
	realcoli += 0.5*origprevimagcol->SubVector(1,N-1);
	realcoli += -0.5*itsimagm->col(i+1,1,N-1);
	imagcoli += -0.5*origprevrealcol->SubVector(1,N-1);
	imagcoli += 0.5*itsrealm->col(i+1,1,N-1);
      } else {
	realcoli += -0.5*origprevimagcol->SubVector(1,N-1);
	realcoli += 0.5*itsimagm->col(i+1,1,N-1);
	imagcoli += 0.5*origprevrealcol->SubVector(1,N-1);
	imagcoli += -0.5*itsrealm->col(i+1,1,N-1);
      }
      SWAP(origprevimagcol,origthisimagcol);
    } else {
      if (plus) {
	imagcoli = -0.5*origprevrealcol->SubVector(1,N-1);
	imagcoli += 0.5*itsrealm->col(i+1,1,N-1);
      } else {
	imagcoli = 0.5*origprevrealcol->SubVector(1,N-1);
	imagcoli += -0.5*itsrealm->col(i+1,1,N-1);
      }
    }
    SWAP(origprevrealcol,origthisrealcol);
  }
  itsrealm->row(0) = itsrealm->row(1);
  itsrealm->row(itsrealm->colsize()-1) = itsrealm->row(itsrealm->colsize()-2);
  itsrealm->col(0) = itsrealm->col(1);
  itsrealm->col(itsrealm->rowsize()-1) = itsrealm->col(itsrealm->rowsize()-2);
  itsimagm->row(0) = itsimagm->row(1);
  itsimagm->row(itsimagm->colsize()-1) = itsimagm->row(itsimagm->colsize()-2);
  itsimagm->col(0) = itsimagm->col(1);
  itsimagm->col(itsimagm->rowsize()-1) = itsimagm->col(itsimagm->rowsize()-2);
  delete origprevrealcol;
  delete origthisrealcol;
  if (iscomplex) {
    delete origprevimagcol;
    delete origthisimagcol;
  } else iscomplex = true;
  xxdbg<<"done\n";
}

void CImage::DPM2(bool plus)
// DPlus^2 (or DPlus2) and DMinus^2 is not really the above convolution twice.
// The second derivative can also be done with a 3x3 matrix, whereas the
// square of these are 5x5 matrices.
//
// So DPlus2 and DMinus2 are:
// [ -I/2  -1  +I/2 ]     [ +I/2  -1  -I/2 ]
// [ +1     0  +1   ] and [ +1     0  +1   ] respectively.
// [ +I/2  -1  -I/2 ]     [ -I/2  -1  +I/2 ]
//
{
  xxdbg<<"Start dpm2 "<<plus<<endl;
  size_t N = itsrealm->colsize();
  Vector<double>* origprevrealcol = new Vector<double>(itsrealm->col(0));
  Vector<double>* origthisrealcol = new Vector<double>(N);
  Vector<double>* prevrealdiff = 
    new Vector<double>(itsrealm->col(0,2,N)-itsrealm->col(0,0,N-2));
  Vector<double>* thisrealdiff = 
    new Vector<double>(itsrealm->col(1,2,N)-itsrealm->col(1,0,N-2));
  Vector<double>* origprevimagcol = iscomplex ? 
    new Vector<double>(itsimagm->col(0)) : 0;
  Vector<double>* origthisimagcol = iscomplex ? 
    new Vector<double>(N) : 0;
  Vector<double>* previmagdiff = iscomplex ? 
    new Vector<double>(itsimagm->col(0,2,N)-itsimagm->col(0,0,N-2)) : 0;
  Vector<double>* thisimagdiff = iscomplex ? 
    new Vector<double>(itsimagm->col(1,2,N)-itsimagm->col(1,0,N-2)) : 0;

  for(size_t i=1;i<itsrealm->rowsize()-1;i++) {
    *origthisrealcol = itsrealm->col(i);
    if (iscomplex) *origthisimagcol = itsimagm->col(i);
    VectorView<double> realcoli = itsrealm->col(i,1,N-1);
    VectorView<double> imagcoli = itsimagm->col(i,1,N-1);
    realcoli = origthisrealcol->SubVector(2,N);
    realcoli += origthisrealcol->SubVector(0,N-2);
    realcoli -= origprevrealcol->SubVector(1,N-1);
    realcoli -= itsrealm->col(i+1,1,N-1);
    if (iscomplex) {
      imagcoli = origthisimagcol->SubVector(2,N);
      imagcoli += origthisimagcol->SubVector(0,N-2);
      imagcoli -= origprevimagcol->SubVector(1,N-1);
      imagcoli -= itsimagm->col(i+1,1,N-1);

      if (plus) {
	realcoli += 0.5*(*previmagdiff);
	imagcoli += -0.5*(*prevrealdiff);
	*prevrealdiff = itsrealm->col(i+1,2,N)-itsrealm->col(i+1,0,N-2);
	*previmagdiff = itsimagm->col(i+1,2,N)-itsimagm->col(i+1,0,N-2);
	// Now prevdiff is really nextdiff
	realcoli += -0.5*(*previmagdiff);
	imagcoli += 0.5*(*prevrealdiff);
      } else {
	realcoli += -0.5*(*previmagdiff);
	imagcoli += 0.5*(*prevrealdiff);
	*prevrealdiff = itsrealm->col(i+1,2,N)-itsrealm->col(i+1,0,N-2);
	*previmagdiff = itsimagm->col(i+1,2,N)-itsimagm->col(i+1,0,N-2);
	// Now prevdiff is really nextdiff
	realcoli += 0.5*(*previmagdiff);
	imagcoli += -0.5*(*prevrealdiff);
      }
      SWAP(origprevimagcol,origthisimagcol);
      SWAP(previmagdiff,thisimagdiff);
    } else {
      if (plus) {
	imagcoli = -0.5*(*prevrealdiff);
	*prevrealdiff = itsrealm->col(i+1,2,N)-itsrealm->col(i+1,0,N-2);
	imagcoli += 0.5*(*prevrealdiff);
      } else {
	imagcoli = 0.5*(*prevrealdiff);
	*prevrealdiff = itsrealm->col(i+1,2,N)-itsrealm->col(i+1,0,N-2);
	imagcoli += -0.5*(*prevrealdiff);
      }
    }
    SWAP(origprevrealcol,origthisrealcol);
    SWAP(prevrealdiff,thisrealdiff);
  }
  itsrealm->row(0) = itsrealm->row(1);
  itsrealm->row(itsrealm->colsize()-1) = itsrealm->row(itsrealm->colsize()-2);
  itsrealm->col(0) = itsrealm->col(1);
  itsrealm->col(itsrealm->rowsize()-1) = itsrealm->col(itsrealm->rowsize()-2);
  itsimagm->row(0) = itsimagm->row(1);
  itsimagm->row(itsimagm->colsize()-1) = itsimagm->row(itsimagm->colsize()-2);
  itsimagm->col(0) = itsimagm->col(1);
  itsimagm->col(itsimagm->rowsize()-1) = itsimagm->col(itsimagm->rowsize()-2);
  delete origprevrealcol;
  delete origthisrealcol;
  delete prevrealdiff;
  delete thisrealdiff;
  if (iscomplex) {
    delete origprevimagcol;
    delete origthisimagcol;
    delete previmagdiff;
    delete thisimagdiff;
  } else iscomplex = true;
  xxdbg<<"done\n";
}

void CImage::DPM0()
// Also, DPlusMinus is:
// [ 0   1   0 ]
// [ 1  -4   1 ]
// [ 0   1   0 ]
//
{
  xxdbg<<"Start dpm0\n";
  size_t N = itsrealm->colsize();
  Vector<double>* origprevrealcol = new Vector<double>(itsrealm->col(0));
  Vector<double>* origthisrealcol = new Vector<double>(N);
  Vector<double>* origprevimagcol = iscomplex ? 
    new Vector<double>(itsimagm->col(0)) : 0;
  Vector<double>* origthisimagcol = iscomplex ? 
    new Vector<double>(N) : 0;

  for(size_t i=1;i<itsrealm->rowsize()-1;i++) {
    *origthisrealcol = itsrealm->col(i);
    if (iscomplex) *origthisimagcol = itsimagm->col(i);
    VectorView<double> realcoli = itsrealm->col(i,1,N-1);
    VectorView<double> imagcoli = itsimagm->col(i,1,N-1);
    realcoli *= double(-4.);
    realcoli += origprevrealcol->SubVector(1,N-1);
    realcoli += origthisrealcol->SubVector(0,N-2);
    realcoli += origthisrealcol->SubVector(2,N);
    realcoli += itsrealm->col(i+1,1,N-1);
    if (iscomplex) {
      imagcoli *= double(-4.);
      imagcoli += origprevimagcol->SubVector(1,N-1);
      imagcoli += origthisimagcol->SubVector(0,N-2);
      imagcoli += origthisimagcol->SubVector(2,N);
      imagcoli += itsimagm->col(i+1,1,N-1);
      SWAP(origprevimagcol,origthisimagcol);
    }
    SWAP(origprevrealcol,origthisrealcol);
  }
  itsrealm->row(0) = itsrealm->row(1);
  itsrealm->row(itsrealm->colsize()-1) = itsrealm->row(itsrealm->colsize()-2);
  itsrealm->col(0) = itsrealm->col(1);
  itsrealm->col(itsrealm->rowsize()-1) = itsrealm->col(itsrealm->rowsize()-2);
  delete origprevrealcol;
  delete origthisrealcol;
  if (iscomplex) {
    itsimagm->row(0) = itsimagm->row(1);
    itsimagm->row(itsimagm->colsize()-1) = itsimagm->row(itsimagm->colsize()-2);
    itsimagm->col(0) = itsimagm->col(1);
    itsimagm->col(itsimagm->rowsize()-1) = itsimagm->col(itsimagm->rowsize()-2);
    delete origprevimagcol;
    delete origthisimagcol;
  }
}

void CImage::DPM1Alt(bool plus)
// There are also alternate versions of DPlus, DMinus and DPlusMinus
//
// DPlusAlt and DMinusAlt are:
//
// [ -1/4+I/4  0  1/4+I/4 ]       [ -1/4-I/4  0  1/4-I/4 ]
// [     0     0     0    ]  and  [     0     0     0    ]
// [ -1/4-I/4  0  1/4-I/4 ]       [ -1/4+I/4  0  1/4+I/4 ]
//
{
  xxdbg<<"Start dpm1 alt "<<plus<<endl;
  if (!iscomplex) {
    itsimagm->Zero();
    iscomplex = true;
  }
  complex<double> im2020 =
    complex<double>(-0.25,-0.25) * complex<double>( (*itsrealm)(19,19) , (*itsimagm)(19,19) ) +
    complex<double>(0.25,-0.25) * complex<double>( (*itsrealm)(19,21) , (*itsimagm)(19,21) ) +
    complex<double>(-0.25,0.25) * complex<double>( (*itsrealm)(21,19) , (*itsimagm)(21,19) ) +
    complex<double>(0.25,0.25) * complex<double>( (*itsrealm)(21,21) , (*itsimagm)(21,21) );
  size_t N = itsrealm->colsize();
  Vector<double>* origprevrealcol = new Vector<double>(itsrealm->col(0));
  Vector<double>* origthisrealcol = new Vector<double>(N);
  Vector<double>* prevrealdiff = 
    new Vector<double>(itsrealm->col(0,2,N)-itsrealm->col(0,0,N-2));
  Vector<double>* thisrealdiff = 
    new Vector<double>(itsrealm->col(1,2,N)-itsrealm->col(1,0,N-2));
  Vector<double>* prevrealsum = 
    new Vector<double>(itsrealm->col(0,2,N)+itsrealm->col(0,0,N-2));
  Vector<double>* thisrealsum = 
    new Vector<double>(itsrealm->col(1,2,N)+itsrealm->col(1,0,N-2));
  Vector<double>* origprevimagcol = iscomplex ? 
    new Vector<double>(itsimagm->col(0)) : 0;
  Vector<double>* origthisimagcol = iscomplex ? 
    new Vector<double>(N) : 0;
  Vector<double>* previmagdiff = iscomplex ? 
    new Vector<double>(itsimagm->col(0,2,N)-itsimagm->col(0,0,N-2)) : 0;
  Vector<double>* thisimagdiff = iscomplex ? 
    new Vector<double>(itsimagm->col(1,2,N)-itsimagm->col(1,0,N-2)) : 0;
  Vector<double>* previmagsum = iscomplex ? 
    new Vector<double>(itsimagm->col(0,2,N)+itsimagm->col(0,0,N-2)) : 0;
  Vector<double>* thisimagsum = iscomplex ? 
    new Vector<double>(itsimagm->col(1,2,N)+itsimagm->col(1,0,N-2)) : 0;

  for(size_t i=1;i<itsrealm->rowsize()-1;i++) {
    *origthisrealcol = itsrealm->col(i);
    if (iscomplex) *origthisimagcol = itsimagm->col(i);
    VectorView<double> realcoli = itsrealm->col(i,1,N-1);
    VectorView<double> imagcoli = itsimagm->col(i,1,N-1);
    realcoli = 0.25*(*prevrealdiff);
    *prevrealdiff = itsrealm->col(i+1,2,N)-itsrealm->col(i+1,0,N-2);
    // Now prevdiff is really nextdiff
    realcoli += 0.25*(*prevrealdiff);
    if (iscomplex) {
      imagcoli = 0.25*(*previmagdiff);
      *previmagdiff = itsimagm->col(i+1,2,N)-itsimagm->col(i+1,0,N-2);
      // Now prevdiff is really nextdiff
      imagcoli += 0.25*(*previmagdiff);
      if (plus) {
	realcoli += 0.25*(*previmagsum);
	imagcoli += -0.25*(*prevrealsum);
	*prevrealsum = itsrealm->col(i+1,2,N)+itsrealm->col(i+1,0,N-2);
	*previmagsum = itsimagm->col(i+1,2,N)+itsimagm->col(i+1,0,N-2);
	// Now prevsum is really nextsum
	realcoli += -0.25*(*previmagsum);
	imagcoli += 0.25*(*prevrealsum);
      } else {
	realcoli += -0.25*(*previmagsum);
	imagcoli += 0.25*(*prevrealsum);
	*prevrealsum = itsrealm->col(i+1,2,N)+itsrealm->col(i+1,0,N-2);
	*previmagsum = itsimagm->col(i+1,2,N)+itsimagm->col(i+1,0,N-2);
	realcoli += 0.25*(*previmagsum);
	imagcoli += -0.25*(*prevrealsum);
      }
      SWAP(origprevimagcol,origthisimagcol);
      SWAP(previmagsum,thisimagsum);
      SWAP(previmagdiff,thisimagdiff);
    } else {
      if (plus) {
	imagcoli = -0.25*(*prevrealsum);
	*prevrealsum = itsrealm->col(i+1,2,N)+itsrealm->col(i+1,0,N-2);
	imagcoli += 0.25*(*prevrealsum);
      } else {
	imagcoli = 0.25*(*prevrealsum);
	*prevrealsum = itsrealm->col(i+1,2,N)+itsrealm->col(i+1,0,N-2);
	imagcoli += -0.25*(*prevrealsum);
      }
    }
    SWAP(origprevrealcol,origthisrealcol);
    SWAP(prevrealsum,thisrealsum);
    SWAP(prevrealdiff,thisrealdiff);
  }
  itsrealm->row(0) = itsrealm->row(1);
  itsrealm->row(itsrealm->colsize()-1) = itsrealm->row(itsrealm->colsize()-2);
  itsrealm->col(0) = itsrealm->col(1);
  itsrealm->col(itsrealm->rowsize()-1) = itsrealm->col(itsrealm->rowsize()-2);
  itsimagm->row(0) = itsimagm->row(1);
  itsimagm->row(itsimagm->colsize()-1) = itsimagm->row(itsimagm->colsize()-2);
  itsimagm->col(0) = itsimagm->col(1);
  itsimagm->col(itsimagm->rowsize()-1) = itsimagm->col(itsimagm->rowsize()-2);
  delete origprevrealcol;
  delete origthisrealcol;
  delete prevrealdiff;
  delete thisrealdiff;
  delete prevrealsum;
  delete thisrealsum;
  if (iscomplex) {
    delete origprevimagcol;
    delete origthisimagcol;
    delete previmagdiff;
    delete thisimagdiff;
    delete previmagsum;
    delete thisimagsum;
  } else iscomplex = true;
  xxdbg<<"done\n";
}

void CImage::DPM0Alt()
// DPlusMinusAlt is:
//
// [ 1/2  0  1/2 ]
// [  0  -2   0  ]
// [ 1/2  0  1/2 ]
//
{
  xxdbg<<"Start dpm0 alt\n";
  size_t N = itsrealm->colsize();
  Vector<double>* origprevrealcol = new Vector<double>(itsrealm->col(0));
  Vector<double>* origthisrealcol = new Vector<double>(N);
  Vector<double>* prevrealsum = 
    new Vector<double>(itsrealm->col(0,2,N)+itsrealm->col(0,0,N-2));
  Vector<double>* thisrealsum = 
    new Vector<double>(itsrealm->col(1,2,N)+itsrealm->col(1,0,N-2));
  Vector<double>* origprevimagcol = iscomplex ? 
    new Vector<double>(itsimagm->col(0)) : 0;
  Vector<double>* origthisimagcol = iscomplex ? 
    new Vector<double>(N) : 0;
  Vector<double>* previmagsum = iscomplex ?
    new Vector<double>(itsimagm->col(0,2,N)+itsimagm->col(0,0,N-2)) : 0;
  Vector<double>* thisimagsum = iscomplex ?
    new Vector<double>(itsimagm->col(1,2,N)+itsimagm->col(1,0,N-2)) : 0;

  for(size_t i=1;i<itsrealm->rowsize()-1;i++) {
    *origthisrealcol = itsrealm->col(i);
    if (iscomplex) *origthisimagcol = itsimagm->col(i);
    VectorView<double> realcoli = itsrealm->col(i,1,N-1);
    VectorView<double> imagcoli = itsimagm->col(i,1,N-1);
    realcoli *= double(-2.);
    realcoli += 0.5*(*prevrealsum);
    *prevrealsum = itsrealm->col(i+1,2,N)+itsrealm->col(i+1,0,N-2);
    // prevsum is now nextsum
    realcoli += 0.5*(*prevrealsum);
    if (iscomplex) {
      imagcoli *= double(-2.);
      imagcoli += 0.5*(*previmagsum);
      *previmagsum = itsimagm->col(i+1,2,N)+itsimagm->col(i+1,0,N-2);
      // prevsum is now nextsum
      imagcoli += 0.5*(*previmagsum);
      SWAP(origprevimagcol,origthisimagcol);
      SWAP(previmagsum,thisimagsum);
    }
    SWAP(origprevrealcol,origthisrealcol);
    SWAP(prevrealsum,thisrealsum);
  }
  itsrealm->row(0) = itsrealm->row(1);
  itsrealm->row(itsrealm->colsize()-1) = itsrealm->row(itsrealm->colsize()-2);
  itsrealm->col(0) = itsrealm->col(1);
  itsrealm->col(itsrealm->rowsize()-1) = itsrealm->col(itsrealm->rowsize()-2);
  delete origprevrealcol;
  delete origthisrealcol;
  delete prevrealsum;
  delete thisrealsum;
  if (iscomplex) {
    itsimagm->row(0) = itsimagm->row(1);
    itsimagm->row(itsimagm->colsize()-1) = itsimagm->row(itsimagm->colsize()-2);
    itsimagm->col(0) = itsimagm->col(1);
    itsimagm->col(itsimagm->rowsize()-1) = itsimagm->col(itsimagm->rowsize()-2);
    delete origprevimagcol;
    delete origthisimagcol;
    delete previmagsum;
    delete thisimagsum;
  }
}


