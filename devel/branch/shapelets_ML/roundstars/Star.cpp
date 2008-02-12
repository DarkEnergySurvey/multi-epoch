#include "Star.h"
#include <cmath>
#include "BinomFact.h"
#include "TMV.h"
#include "TMV_Diag.h"
#include "dbg.h"
#include <vector>

using std::endl;
using tmv::Matrix;
using tmv::Vector;
using std::complex;
using std::vector;

template <class T> inline T SQR(const T& x) { return x*x; }
const double PI = 3.141592653589793; 
const double SQRTPI = sqrt(PI);

double tr_a,tr_b,tr_c,tr_d; 
// The transformation to the coords in which to measure the shape

bool Star::FindKernel(Kernel* k)
{
  xdbg<<"start find kernel\n";
  size_t order = k->GetOrder();
  FullShape initshape(order),convshape(order);
  static vector<FullShape*> allshape;
  static bool first = true;
  if (first) {
    for(size_t i=0;i<params.maxkerneliter;i++)
      allshape.push_back(new FullShape((i+2)*order));
    first = false;
  }
  static Kernel kernel(order);

  FullShape shape(order);
  xdbg<<"before measure shape\n";

  if (!MeasureShape(shape,true)) {
    dbg<<"Initial measure shape returned false\n";
    return false;
  }
  xdbg<<"done initial measure shape:\n";
  if(XDEBUG) shape.Write(*dbgout,kernel.GetOrder());
  double newsigsq = (GetIxx()+GetIyy())/2.;
  xdbg<<"initsigsq = "<<newsigsq<<", e = "<<
    shape.Get(2,0)/shape.Get(0,0)<<endl;
  // temp = 1+e
  double temp = 1+sqrt(norm(shape.Get(2,0))/norm(shape.Get(0,0)));
  xdbg<<"temp = "<<temp<<endl;
  newsigsq *= temp*temp;
  xdbg<<"newsigsq = "<<newsigsq<<endl;
  SetIxx(newsigsq);
  SetIyy(newsigsq);
  size_t iter;
  for(iter=0;iter<params.maxkerneliter;iter++) {
    FullShape& shape(*allshape[iter]);
    dbg<<"iter = "<<iter<<", shapeorder = "<<shape.GetOrder()<<endl;

    // Only fit for x,y,sigsq for first pass
    if (!MeasureShape(shape,false)) {
      dbg<<"Measure shape returned false\n";
      return false;
    }
    xdbg<<"done measure shape:\n";
    if(XDEBUG) shape.Write(*dbgout,kernel.GetOrder());
    if(iter==0) {
      xdbg<<"initshape "<<x<<' '<<y<<"  "<<
          shape.Get(2,0).real()/shape.Get(0,0).real() << ' ' << 
          shape.Get(2,0).imag()/shape.Get(0,0).real() << endl;
    }

    shape.FindAnalyticKernel(&kernel);
    xdbg<<"found analytic kernel: " << kernel;

    // Always apply the kernel from the initial star
    Star convstar(*this);
    convstar.MaskAperture(); // Sets to 0 all values outside of aperture
    convstar *= kernel;
    if (XDEBUG) {
      if (!convstar.MeasureShape(convshape,false)) 
	myerror("convstar MeasureShape returned false");
      xdbg<<"measured convstar shape\n";
      FullShape predshape(shape);
      xdbg<<"made predshape\n";
      predshape *= kernel;
      xdbg<<"i,j  start  predicted  measured\n";
      for(size_t j=0;j<=4;j++) for(size_t i=j;i+j<=4;i++) 
        xdbg<<i<<','<<j<<"  "<<shape.Get(i,j)<<"  "<<
          predshape.Get(i,j)<<"  "<<convshape.Get(i,j)<<endl;
      xdbg<<"convolved shape (without recentering/resizing)\n";
      convstar.MeasureShape(convshape,false);
      convshape.Write(*dbgout,kernel.GetOrder());
      xdbg<<"convolved shape (with recenvering/resizing)\n";
      convstar.MeasureShape(convshape,true);
      convshape.Write(*dbgout,kernel.GetOrder());
    }
    if (!convstar.MeasureShape(convshape,true,true)) continue;
    double rtol = 
      (iter == params.maxkerneliter-1 && params.roundnesstol2 > 0) ?
      params.roundnesstol2 : params.roundnesstol;
    dbg<<"params.roundnesstol = "<<params.roundnesstol<<", tol2 = "<<params.roundnesstol2<<", rtol = "<<rtol<<endl;
    if (convshape.IsRound(kernel.GetOrder(),rtol)) {
      *k = kernel; 
      xdbg<<"convshape "<<x<<' '<<y<<' '<<
        convshape.Get(2,0).real()/convshape.Get(0,0).real() << ' ' << 
        convshape.Get(2,0).imag()/convshape.Get(0,0).real() <<endl;
      return true;
    }
    // Set new size,pos for next pass
    SetX(convstar.GetX());
    SetY(convstar.GetY());
    double newsigsq = convstar.GetIxx();
    SetIxx(newsigsq);
    SetIyy(newsigsq);
  }
  dbg<<"Shape didn't converge to round for star at "<<x<<','<<y<<endl;
  return false;
}


// Helper functions and global variables for MeasureShape:
const Star* globalstar = 0;
bool dosubpix;
bool fitforsky;
size_t npixdiv,starti,startj,endi,endj;
double dnpixdiv,startoffset,sigmaxsubinterval;
vector<vector<bool> > inaperture1,inaperture2;

void CalcLaguerre(Matrix<double,tmv::ColMajor>* L, double x, size_t order);
void CalcLaguerre(Matrix<double,tmv::ColMajor>* L, double x, size_t order,
    size_t m);
bool NewGlobalStar(const Star&,const Vector<double>& xysig);
void CalcZeroFuncs(const Vector<double>& xysig,Vector<double>& f);
void CalcJacobian(const Vector<double>& xysig,const Vector<double>& f,
    Matrix<double,tmv::ColMajor>& df);
bool newt(Vector<double>& x,
    void (*vecfunc)(const Vector<double>&,Vector<double>&),
    void (*fdjac)(const Vector<double>&,const Vector<double>&,
      Matrix<double,tmv::ColMajor>&));

bool Star::MeasureShape(FullShape& shape, bool donewt, bool forisround)
{
    xdbg<<"Measure shape for star at "<<GetX()<<','<<GetY()<<endl;

    double icenter=0,jcenter=0,sigsq=0,sky=0,sigma=0;

    if (donewt) {
        // Initialize xysig to values in star
        Vector<double> xysig(4);
        xdbg<<"GetXMin = "<<GetXMin()<<", GetYMin = "<<GetYMin()<<endl;
        xdbg<<"GetXMax = "<<GetXMax()<<", GetYMax = "<<GetYMax()<<endl;
        Assert(GetX() > GetXMin());
        Assert(GetY() > GetYMin());
        xysig[0] = GetX()-GetXMin()-1;
        xysig[1] = GetY()-GetYMin()-1;
        xysig[2] = log((GetIxx()+GetIyy())/2.);
        xysig[3] = GetSky();
        fitforsky = params.fitforsky; // want to be able to turn it off...

        if(!NewGlobalStar(*this,xysig)) {
            dbg<<"NewGlobalStar indicates that the star is either too close\n";
            dbg<<"to the edge or else it is too close to another bright object.\n";
            return false;
        }
        Vector<double> xysiginit(xysig);
        Vector<double> f(4);
        if (XDEBUG) CalcZeroFuncs(xysig,f);
        xdbg<<"before newt: xysig = "<<xysig<<endl;
        xdbg<<"before newt: f = "<<f<<endl;

        size_t iter;
        for (iter=0;iter<params.maxmeasureiter;iter++) {
            Vector<double> xysigbefore(xysig);
            xdbg<<"iter = "<<iter<<endl;
            if (!newt(xysig,CalcZeroFuncs,CalcJacobian)) {
                dbg<<"Newt didn't converge.  Bailing.\n";
                return false;
            }
            xdbg<<"done newt\n";

            if (XDEBUG) CalcZeroFuncs(xysig,f);
            xdbg<<"after newt: x = "<<xysig<<endl;
            xdbg<<"after newt: f = "<<f<<endl;

            // Check to see how much the parameters changed.  If they change
            // too much, it probably hasn't converged properly, since the
            // values "inaperture" will be different with the new parameters.
            double change = SQR(xysigbefore[0]-xysig[0])+SQR(xysigbefore[1]-xysig[1]);
            double pixscalesq = std::abs(tr_a*tr_d-tr_b*tr_c); // (arcsec/pixel)^2
            if (change > params.maxwandersq/pixscalesq) {
                dbg<<"center wandered: x: "<<xysiginit[0]<<"->"<<xysig[0]<<
                    ", y: "<<xysiginit[1]<<"->"<<xysig[1]<<endl;
                dbg<<"max change = "<<sqrt(params.maxwandersq)<<" arcsec = ";
                dbg<<sqrt(params.maxwandersq/pixscalesq)<<" pixels\n";
                return false;
            }
            change /= 0.25;  // change of 0.5 pixels is significant
            change += SQR(xysig[2]-xysigbefore[2])/0.04;
            // change of 20% is significant
            xdbg<<"change = "<<change<<endl;
            if (change < 1.) {
                SetX(GetXMin() + xysig[0] +1);
                SetY(GetYMin() + xysig[1] +1);
                SetIxx(exp(xysig[2]));
                SetIyy(exp(xysig[2]));
                SetIxy(0.);
                SetSky(xysig[3]);
                break;
            }

            // If sigma blows up, stop fitting for the sky, and just
            // use the initial value from either sextractor or the
            // previous pass in FindKernel
            if (xysig[2]>=params.maxlnsigsq) {
                if (!fitforsky) {
                    dbg<<"Not fitting for sky, and sigsq still blew up.\n";
                    return false;
                }
                fitforsky=false;
                xysig = xysiginit;
            }
            if(!NewGlobalStar(*this,xysig)) {dbg<<"!NewGlobalStar\n"; return false;}
            xdbg<<change<<" not close enough for convergence... repeat (iter="<<
                iter+1<<").\n";
        }

        // Check to make sure it converged in a reasonable manner
        if (iter==params.maxmeasureiter) {
            dbg<<"maxmeasureiter exceeded\n";
            return false;
        }
        if (SQR(xysiginit[0]-xysig[0]) + SQR(xysiginit[1]-xysig[1])
                > params.maxwandersq) {
            dbg<<"center wandered: x: "<<xysiginit[0]<<"->"<<xysig[0]<<
                ", y: "<<xysiginit[1]<<"->"<<xysig[1]<<endl;
            return false;
        }

        // That seems to be a good fit, so find all the other coefficients
        dbg<<"Got a good fit with x,y,s^2,sky = "<<xysig[0]<<' '<<
            xysig[1]<<' '<<exp(xysig[2])<<' '<<xysig[3]<<endl;
        icenter = xysig[0]; jcenter = xysig[1]; 
        sigsq = exp(xysig[2]); sky = xysig[3];
        sigma=sqrt(sigsq);
        shape.SetSigma(sigma);
        shape.SetJacobian(tr_a,tr_b,tr_c,tr_d);
    } else {  // don't do newt
        if (!globalstar) {
            Vector<double> xysig(4);
            xysig[0] = GetX()-GetXMin()-1;
            xysig[1] = GetY()-GetYMin()-1;
            xysig[2] = log((GetIxx()+GetIyy())/2.);
            xysig[3] = GetSky();
            if(!NewGlobalStar(*this,xysig)) {dbg<<"!NewGlobalStar\n"; return false;}
        }
        icenter = GetX()-GetXMin()-1;
        jcenter = GetY()-GetYMin()-1;
        sigsq = (GetIxx()+GetIyy())/2.;
        sky = GetSky();
        sigma = sqrt(sigsq);
        shape.SetSigma(sigma);
        shape.SetJacobian(tr_a,tr_b,tr_c,tr_d);
    }

    xdbg<<"starting to measure: sigma = "<<sigma<<endl;
    xdbg<<"icenter = "<<icenter<<", jcenter = "<<jcenter<<endl;
    xdbg<<"sky = "<<sky<<", jac = "<<shape.GetJacobian()<<endl;

    // Clear the shape matrix
    size_t order = shape.GetOrder();
    for(size_t q=0;q<=order;q++) for(size_t p=q;p+q<=order;p++)
        shape.Set(p,q,0.);

    // For each i,j add to the each shape coefficient the appropriate term in
    // the sum: <pq|F> (Note that <pq| is conjugate of |pq>, hence the -i )
    // = Sum_ij [F(i,j) *
    //   exp(-r^2/2s^2) r^abs(p-q) exp(-i(p-q)phi) L~(min(p,q),abs(p-q)) ]
    // Note that this neglects the 1/sigma sqrt(Pi p! q!) in front of <pq|
    // This is put in below.

    Matrix<double,tmv::ColMajor> L(order/2+1,order+1,0.);
    double dxdy = 1./SQR(dnpixdiv);
    double subinterval = dosubpix ? sigmaxsubinterval/sigma : 0.;

    forisround &= (params.roundnesslevel == 2 || params.roundnesslevel == 5);

    // We really want dx'dy', not dxdy, so multiply by the Jacobian
    // of the transformation
    dxdy *= std::abs(tr_a*tr_d-tr_b*tr_c);

    xdbg<<"i range = "<<starti<<','<<endi<<endl;
    xdbg<<"j range = "<<startj<<','<<endj<<endl;

    Vector<complex<double> >& shapec = shape.GetCoeffs();
    size_t shapes = shapec.size();

    for(size_t i=starti;i<=endi;i++) {
        double dx = i - startoffset - icenter;
        const vector<bool>& inap1i = inaperture1[i];
        tmv::ConstVectorView<double> rowi = this->GetM().row(i);
        dx /= sigma;
        for(size_t subi=0,api=npixdiv*i;subi<npixdiv;subi++,dx+=subinterval,api++) {
            const vector<bool>& inap2i = inaperture2[api];
            for(size_t j=startj;j<=endj;j++) if (inap1i[j]) {
                double imij = rowi(j);
                double dy = j - startoffset - jcenter;
                dy /= sigma;
                for(size_t subj=0,apj=npixdiv*j;subj<npixdiv;subj++,dy+=subinterval,apj++) {
                    if (inap2i[apj]) {
                        double dxprime = tr_a*dx+tr_b*dy;
                        double dyprime = tr_c*dx+tr_d*dy;
                        double rsq = SQR(dxprime)+SQR(dyprime);
                        double weight = exp(-rsq/2.);

                        // z = r exp(-i phi)/sigma
                        //   = (x - iy)/sigma where x and y are measured from the center
                        // (The minus sign is because it is <pq|, not |pq>)
                        complex<double> z(dxprime,-dyprime);

                        double wI = (imij-sky)*weight;
                        if(forisround) {
                            // First (0,0) term - no other m=0
                            //Assert(0 == FSIJ(0,0));
                            shape[0] += wI;

                            // Next, all m=2 terms
                            int q = (order-2)/2;
                            CalcLaguerre(&L,rsq,order,2);
                            complex<double> wIzsq = wI*SQR(z);
                            tmv::CVIt<double,tmv::Unit,tmv::NonConj> L_q_2 =
                                L.col(2).begin()+q;
                            size_t fsijpq = (q+4)*q+2;
                            size_t delta = 2*q+3;
                            tmv::VIt<complex<double>,tmv::Unit,tmv::NonConj> shapeit =
                                shapec.begin()+fsijpq;
                            //for(;q>=0;q--,L_q_2--,fsijpq-=delta,shapeit-=delta,delta-=2) 
                            for(;q>=0;q--,L_q_2--,shapeit-=delta,delta-=2) {
                                //Assert(fsijpq = FSIJ(q+2,q));
                                //Assert(shapeit-shapec.begin()==fsijpq);
                                //Assert(fsijpq < shapes);
                                *shapeit += wIzsq*(*L_q_2);
                            }
                        }
                        else {
                            CalcLaguerre(&L,rsq,order);
                            complex<double> wIzm = wI; // wIzm = w * I * z^m
                            for(size_t m=0;m<=order;m++) { // m = abs(p-q)
                                if (m>0) wIzm *= z;
                                int q = (order-m)/2;
                                //int p = q+m;
                                size_t fsijpq = q*(q+m+2)+(m+1)*(m+1)/4;
                                size_t delta = 2*q+m+1;
                                tmv::CVIt<double,tmv::Unit,tmv::NonConj> L_q_m =
                                    L.col(m).begin()+q;
                                tmv::VIt<complex<double>,tmv::Unit,tmv::NonConj> shapeit =
                                    shapec.begin()+fsijpq;
                                //for(;q>=0;p--,q--,L_q_m--,fsijpq-=delta,shapeit-=delta,delta-=2) 
                                for(;q>=0;q--,L_q_m--,shapeit-=delta,delta-=2) {
                                    //Assert(p==q+m);
                                    //Assert(fsijpq == FSIJ(p,q));
                                    //Assert(fsijpq < shapes);
                                    //Assert(shapeit-shapec.begin()==fsijpq);
                                    //complex<double> psi = wIzm * (*L_q_m);
                                    // This is the m > 0 term, so p = q+m
                                    //*shapeit += psi;
                                    *shapeit += wIzm * (*L_q_m);
                                    // This is the m < 0 term, so p = q-m
                                    // Or, alternatively, swap p and q from above.
                                    //if(m>0) shape[FSIJ(q,p)] += conj(temp3);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Multiply each by dxdy/(sigma sqrt(Pi*p!*q!)), a term that was neglected
    // in the above calculation.  (for efficiency)
    shapec *= dxdy/sigma/SQRTPI;
    static Vector<double>* invsqrtpqfact = 0;
    if (!invsqrtpqfact || invsqrtpqfact->size() < shapes) {
        if (invsqrtpqfact) delete invsqrtpqfact;
        invsqrtpqfact = new Vector<double>(shapes);
        for(size_t q=0;q<=order;q++) for(size_t p=q;p+q<=order;p++) {
            (*invsqrtpqfact)[FSIJ(p,q)] = 1. / (sqrtfact(p) * sqrtfact(q));
        }
    }
    shapec *= DiagMatrixViewOf(invsqrtpqfact->SubVector(0,shapes));

    return true;
}

void Star::MaskAperture()
{
  for(size_t i=0;i<=GetMaxI();i++) {
    const vector<bool>& inap1i = inaperture1[i];
    tmv::VectorView<double> rowi = this->GetM().row(i);
    for(size_t j=0;j<=GetMaxJ();j++) if(!inap1i[j]) rowi(j) = GetSky();
  }
}

void CalcLaguerreCol(tmv::VectorView<double> Lcol,
    double x, size_t order, size_t m)
{
  //Assert(Lcol(0) == 1.);
  Assert(Lcol.size() > 1);
  //Assert(Lcol.step()==1);
  //Assert(Lcol.size() == (order-m)/2+1);
  double c1 = m+1.; // c1 = 2*n+m-1
  Lcol(1) = x-c1;
  if (Lcol.size() == 2) return;
  tmv::CVIt<double,tmv::Unit,tmv::NonConj> L_nm2_m = Lcol.begin();
  tmv::CVIt<double,tmv::Unit,tmv::NonConj> L_nm1_m = L_nm2_m+1;
  tmv::VIt<double,tmv::Unit,tmv::NonConj> L_n_m = Lcol.begin()+2;
  double c2 = c1;   // c2 = (n+m-1)*(n-1)
  c1 += 2.;
  *L_n_m = (x-c1) * (*L_nm1_m) - c2 * (*L_nm2_m);
  //size_t nmax = Lcol.size();
  //for(size_t n=2;n<nmax;n++,L_n_m++,L_nm1_m++,L_nm2_m++) {
  for(size_t m=Lcol.size()-3; m > 0; --m) { // n = Lcol.size - m
    ++L_n_m, ++L_nm1_m, ++L_nm2_m;
    c2 += c1; // Note - these must be in this order.
    c1 += 2.;
    *L_n_m = (x-c1) * (*L_nm1_m) - c2 * (*L_nm2_m);
  }
}

void CalcLaguerre(Matrix<double,tmv::ColMajor>* L, double x, size_t order)
// Calculates, not exactly the assiciated Laguerre polynomials at x,
// but rather (-)^n n! L_n^m(x), which I usually notate as L~.
// This is the polynomial that actually appears in chi_pq instead of L.
// The usual recurrence relation is:
// (n+1) L_n+1^m = (2n+m+1-x) L_n^m - (n+m) L_n-1^m
// So for the L~ polynomials:
// L~_n+1^m = (x-(2n+m+1)) L~_n^m - (n+m)*n L~_n-1^m
// Or:
// L~_n^m = (x-(2n+m-1)) L~_n-1^m - (n+m-1)*(n-1) L~_n-2^m
// The starting values for the recurrence are:
// L~_0^m = 1
// L~_1^m = x-m-1
{
  L->row(0).SetAllTo(1.);
  for(int m=order-2;m>=0;m--) {
    CalcLaguerreCol(L->col(m,0,(order-m)/2+1),x,order,m);
  }
}

void CalcLaguerre(Matrix<double,tmv::ColMajor>* L, double x, size_t order,
    size_t m)
// Just calculates the L~ polynomials for the given m
{
  (*L)(0,m) = 1.;
  if (order > m)
    CalcLaguerreCol(L->col(m,0,(order-m)/2+1),x,order,m);
}

bool NewGlobalStar(const Star& star, const Vector<double>& xysig)
{
  globalstar = &star;

  double x0 = xysig[0];
  double y0 = xysig[1];
  double sigsq = exp(xysig[2]);
  double sigma = sqrt(sigsq);
  double sky = xysig[3];
  xdbg<<"xysig = "<<x0<<' '<<y0<<' '<<sigsq<<' '<<sky<<endl;

  double subinterval;

  if (sigsq < params.subpixthresh) {
    xdbg<<"do subpix\n";
    dosubpix = true;
    npixdiv = params.npixdiv;
    dnpixdiv = npixdiv;
    startoffset = (dnpixdiv-1.)/2./dnpixdiv;
    sigmaxsubinterval = 1./dnpixdiv;
    subinterval = sigmaxsubinterval/sigma;
  }
  else {
    xdbg<<"don't do subpix\n";
    dosubpix = false;
    npixdiv = 1;
    dnpixdiv = 1.;
    startoffset = sigmaxsubinterval = subinterval = 0.;
  }

  inaperture1 = vector<vector<bool> >(star.GetMaxI()+1,
      vector<bool>(star.GetMaxJ()+1,false));
  inaperture2 = vector<vector<bool> >((star.GetMaxI()+1)*npixdiv,
      vector<bool>((star.GetMaxJ()+1)*npixdiv,false));
  xdbg<<"done set apertures\n";

  // Measure the moments in the x',y' coordinate system.
  // x' = u(x,y) and y' = v(x,y).
  // params.dudx, .dudy, .dvdx, and .dvdy give the relevant transformation
  // Let a = dudx, b = dudy, c = dvdx, d = dvdy
  // Then with (x',y') set to (0,0) at the same place as (x,y) = (0,0)
  // we have the transformation:
  // x' = ax + by  and y' = cx + dy

  tr_a = (*params.dudx)(star.GetX(),star.GetY());
  tr_b = (*params.dudy)(star.GetX(),star.GetY());
  tr_c = (*params.dvdx)(star.GetX(),star.GetY());
  tr_d = (*params.dvdy)(star.GetX(),star.GetY());

  xdbg<<"tr = "<<tr_a<<' '<<tr_b<<' '<<tr_c<<' '<<tr_d<<endl;

  size_t ix = size_t(x0), iy = size_t(y0);
  double sigmapix = sigma / sqrt(std::abs(tr_a*tr_d-tr_b*tr_c));
  xdbg<<"sigma, sigpix = "<<sigma<<','<<sigmapix<<endl;
  size_t maxd = size_t(sqrt(params.aperturesq)*sigmapix)+1;
  xdbg<<"ix,iy,maxd = "<<ix<<' '<<iy<<' '<<maxd<<endl;
  xdbg<<"Maxi,j = "<<star.GetMaxI()<<"  "<<star.GetMaxJ()<<endl;
  if (maxd > iy || maxd > ix || 
      ix+maxd > star.GetMaxI() || iy+maxd > star.GetMaxJ()) { 
    globalstar = 0; 
    dbg<<"maxd is too large for given patch: maxd = "<<maxd<<endl;
    dbg<<"ix,iy = "<<ix<<","<<iy<<endl;
    dbg<<"maxi,j = "<<star.GetMaxI()<<","<<star.GetMaxJ()<<endl;
    return false; 
  }
  Assert(ix-maxd <= star.GetMaxI()); // like checking >= 0
  Assert(iy-maxd <= star.GetMaxJ()); // since these are size_t, this is 
  Assert(ix+maxd <= star.GetMaxI());
  Assert(iy+maxd <= star.GetMaxJ());

  starti=star.GetMaxI(); endi=0;
  startj=star.GetMaxJ(); endj=0;
  double maxrsq=params.aperturesq;

  for(bool newmaxrsq=true;newmaxrsq;) {
    newmaxrsq=false;
    for(size_t i=ix-maxd;i<=ix+maxd;i++) {
      xxdbg<<"i = "<<i<<endl;
      tmv::ConstVectorView<double> imi = star.GetM().row(i);
      vector<bool>& inap1i = inaperture1[i];
      double dx = i - startoffset - x0;
      dx /= sigma;
      xxdbg<<"dx = "<<dx<<endl;
      for(size_t subi=0,api=npixdiv*i;subi<npixdiv;subi++,dx+=subinterval,api++) {
	xxdbg<<" subi = "<<subi<<endl;
	vector<bool>& inap2i = inaperture2[api];
	tmv::CVIt<double,tmv::Step,tmv::NonConj> imij = imi.begin()+iy-maxd;
        for(size_t j=iy-maxd;j<=iy+maxd;j++,imij++) {
	  xxdbg<<"  j = "<<j<<endl;
          double dy = j - startoffset - y0;
          dy /= sigma;
          for(size_t subj=0,apj=npixdiv*j;subj<npixdiv;subj++,dy+=subinterval,apj++) {
	    xxdbg<<"   subj = "<<subj<<endl;
	    double dxprime = tr_a*dx+tr_b*dy;
            double dyprime = tr_c*dx+tr_d*dy;
            double rsq = SQR(dxprime) + SQR(dyprime);
	    xxdbg<<"    dx',dy' = "<<dxprime<<"  "<<dyprime<<endl;
	    xxdbg<<"    rsq = "<<rsq<<" <? "<<maxrsq<<endl;
            if (rsq < maxrsq) {
	      //Assert(star(i,j) == *imij);
              if (*imij<params.minI) {
                maxrsq = rsq-0.01;
                newmaxrsq = true;
              }
//              else if (rsq>9. && star(i,j)-sky > maxvalue) {
//                if(rsq<crrsq) crrsq = rsq;
//                if(subi==0 && subj==0) {
//                  ncosmicrays++;
//                  if (ncosmicrays > 4) {
//                    maxrsq = crrsq-0.01;
//                    newmaxrsq = true;
//                  }
//                }
//              }  
              else {
		xxdbg<<"      OK"<<endl;
                inap1i[j] = true;
                inap2i[apj] = true;
                if (i<starti) starti=i;
                if (i>endi) endi=i;
                if (j<startj) startj=j;
                if (j>endj) endj=j;
              }
            }
          }
        }
      }
    }
//    if(newmaxrsq && ncosmicrays > 4) { // drop maxr by another sigma 
//      maxrsq -= 2.*sqrt(maxrsq);
//    }
    if(newmaxrsq) {
      xxdbg<<"In NewGlobalStar: maxrsq = "<<maxrsq<<endl;
      if (maxrsq < 9.5) { globalstar = 0; return false; }
      //inaperture1.SetAllValues(false);
      //inaperture2.SetAllValues(false);
      //starti=star.GetMaxI();
      //startj=star.GetMaxJ();
      //endi=endj=0;
    }
  }
  return true;
}

void CalcZeroFuncs(const Vector<double>& xysig,Vector<double>& f)
// Calculate for the star (*globalstar):
// f[0] = Sum (x-x0) I'(x,y) exp(-r^2/2) dxdy / Iw
// f[1] = Sum (y-y0) I'(x,y) exp(-r^2/2) dxdy / Iw
// f[2] = Sum (r^2-1) I'(x,y) exp(-r^2/2) dxdy / Iw
// f[3] = Sum I'(x,y) dxdy / Iw - 2
// where all x,y,r are scaled by sigma,
// I'(x,y) = I(x,y) - sky
// I(x,y) is the intensity of the star at that position
// and Iw = Sum I'(x,y) exp(-r^2/2) dxdy
//
// The input values xysig are:
// xysig[0] = x0, xysig[1] = y0, xysig[2] = ln(sigma^2), xysig[3] = sky
//
{
  xdbg<<"Start CalcZeroFuncs\n";
  xdbg<<"starti..endi = "<<starti<<","<<endi<<endl;
  xdbg<<"startj..endj = "<<startj<<","<<endj<<endl;
  xxdbg<<"npixdiv = "<<npixdiv<<endl;
  xxdbg<<"Im = "<<globalstar->GetM().SubMatrix(starti,endi+1,startj,endj+1)<<endl;
  xxdbg<<"inap = "<<endl;
  for(size_t i=starti;i<endi;i++) {
    for(size_t j=startj;j<endj;j++)
      xxdbg<<(inaperture1[i][j]?1:0);
    xxdbg<<endl;
  }
  xxdbg<<"inap2 = "<<endl;
  for(size_t api=npixdiv*starti;api<npixdiv*endi;api++) {
    for(size_t apj=npixdiv*startj;apj<npixdiv*endj;apj++)
      xxdbg<<(inaperture2[api][apj]?1:0);
    xxdbg<<endl;
  }
  xdbg<<"initial f = "<<f<<endl;
  xdbg<<"initial xysig = "<<xysig<<endl;

  double x0 = xysig[0];
  double y0 = xysig[1];
  if (xysig[2] < -10. || xysig[2] > 10.) { f[2]=100.; return; }
  double sigsq = exp(xysig[2]);
  double sky = xysig[3];
  double sigma = sqrt(sigsq);

  xdbg<<"x0,y0,sigsq,sigma,sky = "<<x0<<","<<y0<<","<<sigsq<<","<<sigma<<","<<sky<<endl;

  // Clear f's
  f.Zero();
  xxdbg<<"cleared f = "<<f<<endl;
  double Iw=0.;
  double subinterval = dosubpix ? sigmaxsubinterval/sigma : 0.;

  for(size_t i=starti;i<=endi;i++) {
    tmv::ConstVectorView<double> imi = globalstar->GetM().row(i);
    const vector<bool>& inap1i = inaperture1[i];
    double dx = i - startoffset - x0;
    dx /= sigma;
    for(size_t subi=0,api=npixdiv*i;subi<npixdiv;subi++,dx+=subinterval,api++) {
      const vector<bool>& inap2i = inaperture2[api];
      tmv::CVIt<double,tmv::Step,tmv::NonConj> imij = imi.begin()+startj;
      for(size_t j=startj;j<=endj;j++,imij++) if (inap1i[j]) {
        double dy = j - startoffset - y0;
        dy /= sigma;
        for(size_t subj=0,apj=npixdiv*j;subj<npixdiv;subj++,dy+=subinterval,apj++) {
          if (inap2i[apj]) {
	    xxdbg<<"Im("<<i<<","<<j<<") = "<<*imij<<" = "<<(*globalstar)(i,j)<<endl;
            xxdbg<<"tr = "<<tr_a<<","<<tr_b<<","<<tr_c<<","<<tr_d<<endl;
            xxdbg<<"dx = "<<dx<<", dy = "<<dy<<endl;
            xxdbg<<"sky = "<<sky<<endl;
            double dxprime = tr_a*dx+tr_b*dy;
            double dyprime = tr_c*dx+tr_d*dy;
            double rsq = SQR(dxprime) + SQR(dyprime);
            double weight = exp(-rsq/2.);
	    //Assert(*imij == (*globalstar)(i,j));
            weight *= *imij - sky;
            f[0] += dxprime*weight;
            f[1] += dyprime*weight;
            f[2] += (rsq-1.)*weight;
            if(fitforsky) f[3] += *imij - sky;
            Iw += weight;
	    xxdbg<<"f -> "<<f<<endl;
          }
        }
      }
    }
  }

  xdbg<<"After loop: f -> "<<f<<endl;
  xdbg<<"Iw = "<<Iw<<endl;
  if (Iw == 0.) return;
  while (std::abs(Iw) < 1.e-10) { f *= 1.e10; Iw *= 1.e10; }
  // Divide each f by Iw
  f /= Iw;
  xdbg<<"After /Iw: f -> "<<f<<endl;
  // Subtract constant terms from f[3]
  if(fitforsky) f[3] -= 2.;
  xdbg<<"Done: f -> "<<f<<endl;
}

void CalcJacobian(const Vector<double>& xysig,const Vector<double>& f,
    Matrix<double,tmv::ColMajor>& df)
// Calculate the Jacobian of the functions calculated above.
// df(i,j) = d(f[i])/d(x[j])
//
// df(0..2,0..2) = Sum I'(x,y) exp(-r^2/2) dxdy/Iw *
//
// [ (dx^2-1-f0 dx)/s dy(dx-f0)/s      ((dx-f0)r^2-dx)/2 ]
// [ dx(dy-f1)/s      (dy^2-1-f1 dy)/s ((dy-f1)r^2-dy)/2 ]
// [ dx(r^2-3-f2)/s   dy(r^2-3-f2)/s   r^2(r^2-3-f2)/2   ]
//
// df(0..3,3) = Sum exp(-r^2/2) dxdy/Iw *
//
// [ f[0]-dx  f[1]-dy  f[2]+1-r^2  (f[3]+2)exp(-r^2/2)-1]
//
// df(3,0) = -(f[3]+2) f[0]/s
// df(3,1) = -(f[3]+2) f[1]/s
// df(3,2) = -(f[3]+2) (f[2]+1)/2
{
  double x0 = xysig[0];
  double y0 = xysig[1];
  if (xysig[2] < -10. || xysig[2] > 10.) { return; }
  double sigsq = exp(xysig[2]);
  double sky = xysig[3];
  double f3p2 = f[3]+2.;
  double f2p1 = f[2]+1.;
  double f2p3 = f[2]+3.;

  // Clear df
  df.Zero();
  // Add up all but the dxdy and the 1/sigma or 1/2sigma^2 factors
  double sigma = sqrt(sigsq);
  double Iw = 0.;
  size_t npix=0;
  double subinterval = dosubpix ? sigmaxsubinterval/sigma : 0.;

  for(size_t i=starti;i<=endi;i++) {
    tmv::ConstVectorView<double> imi = globalstar->GetM().row(i);
    const vector<bool>& inap1i = inaperture1[i];
    double dx = i - startoffset - x0;
    dx /= sigma;
    for(size_t subi=0,api=npixdiv*i;subi<npixdiv;subi++,dx+=subinterval,api++) {
      tmv::CVIt<double,tmv::Step,tmv::NonConj> imij = imi.begin()+startj;
      const vector<bool>& inap2i = inaperture2[api];
      for(size_t j=startj;j<=endj;j++,imij++) if (inap1i[j]) {
        double dy = j - startoffset - y0;
        dy /= sigma;
        for(size_t subj=0,apj=npixdiv*j;subj<npixdiv;subj++,dy+=subinterval,apj++) {
          if (inap2i[apj]) {
            npix++;
            double dxprime = tr_a*dx + tr_b*dy;
            double dyprime = tr_c*dx + tr_d*dy;
            double rsq = SQR(dxprime) + SQR(dyprime);
            double weight = exp(-rsq/2.);

            double dxmf0 = dxprime-f[0];
            double dymf1 = dyprime-f[1];
            if(fitforsky) {
	      //const double v1[4] = { -dxmf0, -dymf1, (f2p1-rsq), f3p2 };
	      //df.col(3) += weight*tmv::VectorViewOf(v1,4);
              df(0,3) -= dxmf0*weight;          // df[0]/dsky
              df(1,3) -= dymf1*weight;          // df[1]/dsky
              df(2,3) += (f2p1-rsq)*weight;     // df[2]/dsky
              df(3,3) += f3p2*weight;           // df[3]/dsky
	      // df(3,3) -= 1.  This is done at the end with -= npix.
            }

	    //Assert(*imij == (*globalstar)(i,j));
            weight *= (*imij) - sky;

            Iw += weight;
	    /*
	    const double v2[3] = { dxmf0, dymf1, rsq-f2p3 };
	    const double v3[3] = { weight*dxprime, weight*dyprime, weight*rsq };

	    df.SubMatrix(0,3,0,3) += 
	      (tmv::VectorViewOf(v2,3) ^ tmv::VectorViewOf(v3,3));
	    df(0,0) -= weight;
	    df(1,1) -= weight;
	    df(0,2) -= v3[0];
	    df(1,2) -= v3[1];
	    */

            double xweight = dxprime*weight;
            double yweight = dyprime*weight;
            double rweight = rsq*weight;
            double temp3 = rsq-f2p3;
            df(0,0) += dxmf0*xweight-weight;    // df[0]/dx
            df(0,1) += dxmf0*yweight;           // df[0]/dy
            df(0,2) += dxmf0*rweight-xweight;   // df[0]/dsigsq
            df(1,0) += dymf1*xweight;           // df[1]/dx
            df(1,1) += dymf1*yweight-weight;    // df[1]/dy
            df(1,2) += dymf1*rweight-yweight;   // df[1]/dsigsq
            df(2,0) += temp3*xweight;           // df[2]/dx
            df(2,1) += temp3*yweight;           // df[2]/dy
            df(2,2) += temp3*rweight;           // df[2]/dsigsq
          }
        }
      }
    }
  }

  // These have actually calculated d/dx' and d/dy', not d/dx and d/dy
  // df/dx = (df/dx')(dx'/dx) + (df/dy')(dy'/dx) = a*dfdx' + c*dfdy'
  // df/dy = (df/dx')(dx'/dy) + (df/dy')(dy'/dy) = b*dfdx' + d*dfdy'
  for(size_t i=0;i<3;i++) {
    double dfdxprime = df(i,0);
    double dfdyprime = df(i,1);
    df(i,0) = tr_a*dfdxprime + tr_c*dfdyprime;
    df(i,1) = tr_b*dfdxprime + tr_d*dfdyprime;
  }

  // Divide each df by Iw sigma for j=0,1
  // or by 2Iw for j=2
  // or by Iw for j=3
  for(size_t i=0;i<3;i++) {
    for(size_t j=0;j<=1;j++) df(i,j) /= Iw*sigma;
    df(i,2) /= Iw*2.;
    if(fitforsky) df(i,3) /= Iw;
  }
  if(fitforsky) {
    df(3,3) -= npix;
    df(3,3) /= Iw;                       // df[3]/dsky
    df(3,0) = -(f[3]+2.)*f[0]/sigma;     // df[3]/dx
    df(3,1) = -(f[3]+2.)*f[1]/sigma;     // df[3]/dy
    df(3,2) = -(f[3]+2.)*(f[2]+1.)/2.;   // df[3]/dsigsq
  }
  else df(3,3) = 1.; // Prevent getting singular matrix in newt
}
#undef MINSIGSQ

#define ALF 1.0e-10
#define TOLX 1.0e-10
static double maxarg1,maxarg2;
#define MAX(a,b) (maxarg1=(a),maxarg2=(b),\
    (maxarg1)>(maxarg2) ? (maxarg1) : (maxarg2))

void lnsrch(const Vector<double>& xold, double fold,
    const Vector<double>& g, Vector<double>& p,
    Vector<double>& x, double* f, double stpmax,
    bool* check, double (*func)(const Vector<double>&))
{
  xxdbg<<"Start lnsrch\n";
  const size_t n = xold.size();
  *check = 0;
  double sum = Norm(p);
  if (sum>stpmax) p *= stpmax/sum;
  double slope = g*p;
  double test = 0.;
  for(size_t i=0;i<n;i++) {
    double temp = std::abs(p[i])/MAX(std::abs(xold[i]),1.);
    if (temp > test) test = temp;
  }
  double alamin=TOLX/test;
  double alam=1.;
  double alam2=0.,tmplam,f2=0.,fold2=0.;
  while(1) {
    xxdbg<<"start loop\n";
    xxdbg<<"xold = "<<xold<<endl;
    xxdbg<<"alam = "<<alam<<endl;
    xxdbg<<"p = "<<p<<endl;
    x = xold + alam*p;
    xxdbg<<"x => "<<xold<<endl;
    *f = (*func)(x);
    xxdbg<<"alam = "<<alam<<" <? "<<alamin<<endl;
    if (alam<alamin) {
      x = xold;
      *check=1;
      return;
    }
    xxdbg<<"f = "<<*f<<" <=? ";
    xxdbg<<fold<<"+"<<ALF<<"*"<<alam<<"*"<<slope;
    xxdbg<<" = "<<fold+ALF*alam*slope<<endl;
    if (!(*f <= 10 || *f >= -10)) 
    { dbg<<"nan found in newt (in Star.cpp)\n"; throw "nan"; }
    if (*f <= fold+ALF*alam*slope) return;
    if (alam == 1.0) tmplam = -slope/(2.0*(*f-fold-slope));
    else {
      double rhs1 = *f-fold-alam*slope;
      double rhs2 = f2-fold2-alam2*slope;
      double a = (rhs1/(alam*alam)-rhs2/(alam2*alam2))/(alam-alam2);
      double b = (-alam2*rhs1/(alam*alam)+alam*rhs2/(alam2*alam2))/(alam-alam2);
      if (a==0.0) tmplam = -slope/(2.0*b);
      else {
        double disc = b*b-3.0*a*slope;
        if (disc < 0.0) myerror("Roundoff problem in lnsrch.");
        else tmplam = (-b+sqrt(disc))/(3.0*a);
      }
      if (tmplam > 0.5*alam) tmplam = 0.5*alam;
    }
    alam2 = alam;
    f2 = *f;
    fold2 = fold;
    alam = MAX(tmplam,0.1*alam);
  }
}

#undef ALF

// These global variables are used to communicate between fmin and newt:
Vector<double>* fvec=0;
void (*nrfuncv)(const Vector<double>& v, Vector<double>& f);

double fmin(const Vector<double>& x)
{
  Assert(fvec);
  (*nrfuncv)(x,*fvec);
  xxdbg<<"in fmin: x = "<<x<<endl<<"fvec = "<<*fvec<<endl;
  return 0.5*NormSq(*fvec);
}

#define MAXITS 200
#define TOLF 1.0e-10
#define TOLMIN 1.0e-10
#define STPMX 100.0

bool newt(Vector<double>& x,
    void (*vecfunc)(const Vector<double>&,Vector<double>&),
    void (*fdjac)(const Vector<double>&,const Vector<double>&,
      Matrix<double,tmv::ColMajor>&))
// See NR Section 9.7
{
  xdbg<<"start newt\n";
  const size_t n = x.size();
  bool check;
  if (fvec && fvec->size() != n) delete fvec;
  if (!fvec) fvec = new Vector<double>(n);
  nrfuncv=vecfunc;
  double f = fmin(x); // Note: fvec has been calculated here by fmin
  double test = fvec->NormInf();
  if (test < 0.01*TOLF) {
    xdbg<<"done newt -- success #1\n";
    return true;
  }
  double norm = Norm(x);
  double stpmax = STPMX*MAX(norm,float(n));
  for (size_t its=0;its<MAXITS;its++) {
    xdbg<<"its = "<<its<<", x = "<<x<<endl;
    Matrix<double,tmv::ColMajor> fjac(n,n,0.);
    fdjac(x,*fvec,fjac);
    xdbg<<"fjac = "<<fjac<<endl;
    Vector<double> g = (*fvec) * fjac;
    Vector<double> xold = x;
    double fold = f;
    Vector<double> p = -(*fvec)/fjac;
    xdbg<<"p = "<<p<<endl;
    xdbg<<"xold = "<<xold<<endl;
    try {
      lnsrch(xold,fold,g,p,x,&f,stpmax,&check,fmin);
    } 
    catch (char* s) {
      dbg<<"caught "<<s<<" from lnsrch\n";
      return false;
    }
    xdbg<<"after lnsrch: f = "<<f<<endl;
    test = fvec->NormInf();
    if (test < TOLF) {
      xdbg<<"done newt -- success #2\n";
      return true;
    }
    if (check) {
      test = 0.;
      double den = MAX(f,0.5*n);
      for(size_t i=0;i<n;i++) {
        double temp = std::abs(g[i])*MAX(std::abs(x[i]),1.)/den;
        if (temp > test) test = temp;
      }
      xdbg<<"done newt -- success?\n";
      return test > TOLMIN ? true : false;
    }
    test = 0.;
    for(size_t i=0;i<n;i++) {
      double temp = (std::abs(x[i]-xold[i]))/MAX(std::abs(x[i]),1.);
      if (temp > test) test = temp;
    }
    if (test < TOLX) {
      xdbg<<"done newt -- success #3\n";
      return true;
    }
  }
  xdbg<<"MAXITS exceeded in newt\n";
  return false;
}

#undef MAXITS
#undef TOLF
#undef TOLMIN
#undef STPMX
#undef TOLX
