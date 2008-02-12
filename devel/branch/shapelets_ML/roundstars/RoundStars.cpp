#include "Star.h"
#include "Kernel.h"
#include "RSParams.h"
#include "FullShape.h"
#include <sstream>
#include <fstream>
#include <iostream>
using std::string;
using std::ifstream;
using std::ofstream;
using std::endl;
using std::cout;
using std::cerr;

#define IJ(i,j) ( ((i)+(j))*((i)+(j)+1)/2+(j) )

std::ostream* dbgout = 0;
bool XDEBUG = false;

RSParameters params;
std::ostream* listout = 0;

void ReadCmdLine(int argc, char **argv,
    string *infile,string *starlist,string *outfile,size_t *order);

int main(int argc, char **argv)
{
  string infile,starlistfile,outfile;
  ReadCmdLine(argc,argv,&infile,&starlistfile,&outfile,&params.outputorder);
  dbg<<"done rdcmdline\n";
  
  Image wholeimage(infile);

  ifstream listin(starlistfile.c_str());
  if (!listin) myerror("opening starlistfile",starlistfile.c_str());
  string line;
  ofstream fout(outfile.c_str());
  if (!fout) myerror("opening outfile",outfile.c_str());
  double x,y,m,ixx,ixy,iyy,sky;
  while (getline(listin,line)) {
    xdbg<<"line = "<<line<<endl;
    if (line[0] == '#') continue;
    std::istringstream linein(line);
    string junk;
    linein >> junk >> x >> y >> m >> junk >> sky >> ixx >> iyy >> ixy;
    if (!linein) myerror("reading starlistfile");
    dbg<<"x,y,m,sky = "<<x<<','<<y<<','<<m<<','<<sky<<endl;
    xdbg<<"ixx,ixy,iyy = "<<ixx<<','<<ixy<<','<<iyy<<endl;
    if (sky<params.minI) continue;

    // Shape comes in distorted.  Undistort it here to give a better starting
    // point when remeasuring.
    double tr_a = (*params.dudx)(x,y);
    double tr_b = (*params.dudy)(x,y);
    double tr_c = (*params.dvdx)(x,y);
    double tr_d = (*params.dvdy)(x,y);
    tmv::Matrix<double> D(2,2); D(0,0)=tr_a; D(0,1)=tr_b; D(1,0)=tr_c; D(1,1)=tr_d;
    tmv::Matrix<double> S(2,2); S(0,0)=ixx; S(0,1)=S(1,0)=ixy; S(1,1)=iyy;
    S = D*S*D.Transpose();
    ixx = S(0,0); ixy = S(1,0); iyy = S(1,1);
    xdbg<<"new ixx,ixy,iyy = "<<ixx<<','<<ixy<<','<<iyy<<endl;

    xdbg<<"D = "<<D<<endl;
    double pixscale = sqrt(fabs(D.Det()));
    xdbg<<"D.Det = "<<D.Det()<<endl;
    dbg<<"pixscale = "<<pixscale<<endl;

    xdbg<<"wholeimage.xmin,max = "<<wholeimage.GetXMin()<<"  "<<wholeimage.GetXMax()<<endl;
    xdbg<<"wholeimage.ymin,max = "<<wholeimage.GetYMin()<<"  "<<wholeimage.GetYMax()<<endl;
    xdbg<<"x,y = "<<x<<"  "<<y<<endl;
    xdbg<<"params.r = "<<params.starradius<<"  "<<params.starradius/pixscale<<endl;
    Star star(wholeimage,x,y,pixscale,ixx,ixy,iyy,sky);
    xdbg<<"Star.xmin,max = "<<star.GetXMin()<<"  "<<star.GetXMax()<<endl;
    xdbg<<"Star.ymin,max = "<<star.GetYMin()<<"  "<<star.GetYMax()<<endl;

    Kernel k(params.outputorder);
    if (star.FindKernel(&k)) {
      dbg<<"found kernel: ";
      fout << x << ' ' << y << ' ' << k;
      dbg<<k;
      if (listout) (*listout) << line << endl;
    }
  }

  if (dbgout && dbgout != &cout) {delete dbgout; dbgout=0;}
  return 0;
}

#include "RequireNext.h"

void ReadCmdLine(int argc, char **argv,
    string *infile,string *starlist,string *outfile,size_t *order)
{
  bool err=false;
  ifstream *distin=0;

  if(argc<5) {
    err=true;
    cerr<<"too few args\n";
  }
  else {
    *infile = argv[1];
    *starlist = argv[2];
    *outfile = argv[3];
    *order = atoi(argv[4]);
    if (!*order) err=true;
    if (err) cerr<<"error in first 4 args\n";
  }

  if(!err) for(int i=5;i<argc && !err;++i) {
    if(argv[i][0] == '-') {
      switch(argv[i][1]) {
        case 'd':
          require_next(dbgout=&cout) {
            if (!(dbgout=new ofstream(argv[i+1]))) err=true;
          }
          dbgout->setf(std::ios_base::unitbuf);
          for(size_t j=2;argv[i][j] != '\0';j++) {
            switch (argv[i][j]) {
              case 'x' : XDEBUG=true; break;
              default : err=true;
            }
          }
          ++i;
	  if (err) cerr<<"error in -d\n";
          break;
        case 'u':
	  require_next(err=true) {
            if (!(distin=new ifstream(argv[++i]))) err=true;
          }
	  if (err) cerr<<"error in -u\n";
          break;
        case 'n':
          if (argv[i][2] != 'm') err=true;
          else params.dominimization = false;
	  if (err) cerr<<"error in -n\n";
          break;
	case 't':
          if (argv[i][2] == '\0') {
	    require_next(err=true) {
              params.roundnesstol = atof(argv[++i]);
              if (params.roundnesstol == 0) err=true;
            }
          } else if (argv[i][2] == '2') {
	    require_next(err=true) {
              params.roundnesstol2 = atof(argv[++i]);
              if (params.roundnesstol2 == 0) err=true;
            }
          } else err=true;
	  if (err) cerr<<"error in -t\n";
          break;
        case 'r':
          require_next(err=true) {
            params.roundnesslevel = atoi(argv[++i]);
            if (params.roundnesslevel == 0) err=true;
          }
	  if (err) cerr<<"error in -r\n";
          break;
        case 'o':
          require_next(err=true) {
            if (!(listout=new ofstream(argv[++i]))) err=true;
          }
	  if (err) cerr<<"error in -o\n";
          break;
	case 'i':
	  require_next(err=true) {
            params.maxkerneliter = atoi(argv[++i]);
            if (params.maxkerneliter == 0) err=true;
          }
	  if (err) cerr<<"error in -i\n";
          break;
	case 'm':
	  require_next(err=true) {
            params.minI = atof(argv[++i]);
          }
	  break;
        default:
          err=true;
	  if (err) cerr<<"error in default\n";
          break;
      }
    }
    else {
      err=true;
      cerr<<"invalid arg: "<<argv[i]<<endl;
    }
  }

  if(err) {
    cerr <<
"Usage: roundstars imagefile starlist starkernelfile order \n\
        [-u undistortfile] [-nm] [-t tol] [-t2 tol2] [-r roundness_level] \n\
        [-o outfile] [-i maxiter] [-m minI] [-d [debugfile]]\n\
The first parameter is the fits image on which to fit the psf \n\
The second parameter is a list of stars from starsep (sextractor format) \n\
The third parameter is the output file with the kernel parameters for each star\n\
The fourth parameter is the order of the kernel to use. \n\
Flags: \n\
    -u Make the stars \"almost round\" so that when the undistortion function\n\
       in undistortfile is applied, the stars will become round. \n\
    -nm Don't use 11,22,40 kernels to minimize the norm of the mask. \n\
    -t Set roundness tolerance (default = 1.e-10) \n\
    -t2 Set roundness tolerance for the last iteration (default = tol) \n\
    -r Set roundness level (default = 6) see code for more details \n\
    -o Ouput stars used to outfile \n\
    -i Set the maximum iterations for finding each kernel (default 3) \n\
    -m Set minimum intensity level for real data (default 0.0) \n\
    -d Output debugging info to debugfile or (if omitted) to stdout \n\
";
    exit(1);
  }

  if (distin) {
    std::auto_ptr<Function2D<double> > u, v;
    *distin >> u >> v;
    params.dudx = u->DFDX();
    params.dudy = u->DFDY();
    params.dvdx = v->DFDX();
    params.dvdy = v->DFDY();
  }
  else { // Default is dudx = 1, dudy = 0, dvdx = 0, dvdy = 1
    params.dudx.reset(new Constant2D<double>(1.));
    params.dudy.reset(new Constant2D<double>(0.));
    params.dvdx.reset(new Constant2D<double>(0.));
    params.dvdy.reset(new Constant2D<double>(1.));
  }
}

