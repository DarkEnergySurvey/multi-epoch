//---------------------------------------------------------------------------
#ifndef UberParamsH
#define UberParamsH
//---------------------------------------------------------------------------

#define ROUNDNESSLEVEL 6    // See comment before LinearFindKernel in Star.cpp
#define MAXKERNALITER 3     // Max iterations for finding kernel
#define MAXMEASUREITER 10   // Max iterations on x,y,sigma for measuring shape
#define NPIXDIV 3           // Number of subpixel divisions if subdivided
#define ROUNDNESSTOL 1.e-10 // Need to see what is reasonable here...
#define MAXWANDER 2.        // Allow 2 arcsec wander
#define SUBPIXTHRESH 4.     // Subdivide pixels if sigma^2 < thresh
#define APSIG 7.            // Aperture size assumed to have good signal
#define MAXSIGMA 2.	    // This corresponds to 5 arcsec seeing.
#define DOMINIMIZATION 1    // 1 to find kernel with minimal norm^2
                            //   by adding m=0 components
#define FITFORSKY 0         // 1 to have MeasureShape fit for the sky value
#define MINI 0.		    // The minimum intensity deemed real

struct RSParameters {
  RSParameters() : roundnesslevel(ROUNDNESSLEVEL),
		   starradius(size_t(MAXSIGMA*APSIG)+3),
		   maxkerneliter(MAXKERNALITER),
		   maxmeasureiter(MAXMEASUREITER),npixdiv(NPIXDIV),
           firstpassorder(0),
		   outputorder(0),
		   roundnesstol(ROUNDNESSTOL),roundnesstol2(0),
		   maxwandersq(MAXWANDER*MAXWANDER),
		   subpixthresh(SUBPIXTHRESH),aperturesq(APSIG*APSIG),
		   minI(MINI),
		   maxlnsigsq(2*log(MAXSIGMA)),

		   dominimization(DOMINIMIZATION),fitforsky(FITFORSKY),

		   dudx(0),dudy(0),dvdx(0),dvdy(0)
  {}

  void SetMaxSize(double newmaxsigma)
  {
    starradius = size_t(newmaxsigma*APSIG)+3;
    maxlnsigsq = 2.*log(newmaxsigma);
  }

  size_t roundnesslevel, starradius, maxkerneliter, maxmeasureiter;
  size_t npixdiv, firstpassorder, outputorder;
  double roundnesstol,roundnesstol2,maxwandersq,subpixthresh,aperturesq;
  double minI;
  double maxlnsigsq; 
  bool dominimization,fitforsky;
  std::auto_ptr<Function2D<double>  > dudx,dudy,dvdx,dvdy;
};

extern RSParameters params;

#endif
