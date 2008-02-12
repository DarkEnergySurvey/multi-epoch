#include "FindStarsLib.h"


std::ostream* dbgout;
bool XDEBUG = false;

// Default values for the parameters used in this program

double minsize=1.0;       // The min and max size to consider
double maxsize=30.0;
double minmag=12.0;       // The min and max magnitude to consider
double maxmag=24.0;
double maxoutmag=24.0;
size_t ndivx=3;           // Divide the image into nx by ny subsections
size_t ndivy=3;

// Parameters when finding stars in each subdivision
size_t startn1=50;        // # objects to start with on first pass 
size_t startn1b=5;        // Of these, how many are probably stars?
double magstep1=0.25;     // Step size in mag for each successive pass
double reject1=2.0;       // nsigma rejection.  (actually n x quartile size)
double maxratio1=0.15;    // max ratio of count in valley / count in peak
double binsize1=0.1;      // binsize of histogram 
size_t okvalcount=2;      // if valley count <= this, ok no matter what peak is
size_t miniter1=3;        // min number of mag steps to take
double maxrms=0.05;       // in initial linear fit, max rms to allow 

// Parameters for last pass of whole image
size_t startn2=200;       // # objects to start with
double magstep2=0.10;     // Step size in mag
double minbinsize=0.01;	  // Min width of histogram bins
double reject2=4.0;       // n quartile rejection
double purityratio=0.10;  // max ratio of count in valley / count in peak
size_t miniter2=2;        // min number of mag steps to take

// Parameters for fitting results of subsections
size_t starsperbin=10;    // How many stars per subsection?
size_t fitorder=2;        // order of fit for size(x,y)
double fitsigclip=4.0;    // nsigma rejection in this fit
size_t maxrefititer=10;   // max number of times to refit whole image

// Parameters for reading input file
size_t xcol=2;            // which column for x,y,m,ixx,iyy,errcode
size_t ycol=3;
size_t mcol=4;
size_t scol1=7;           // scol1,2 are ixx,iyy usually. 
size_t scol2=8;           // if scol2=0, only one column for size
size_t ecol=0;            // if ecol = 0, no errcode
int okerrcode=0;          // errcodes to allow
int logsize=0;		  // 1 if sizes are already log(size)


int main(int argc, char **argv)
{
  string infile,outfile;
  xdbg<<"pre read cmd\n";
  ReadCmdLine(argc,argv,&infile,&outfile);
  xdbg<<"done read cmd line\n";
  ifstream fin(infile.c_str());
  xdbg<<"opened infile: "<<infile<<endl;
  if (!fin) myerror("opening infile");
  size_t maxcol = 0;
  if (xcol > maxcol) maxcol = xcol;
  if (ycol > maxcol) maxcol = ycol;
  if (mcol > maxcol) maxcol = mcol;
  if (scol1 > maxcol) maxcol = scol1;
  if (scol2 > maxcol) maxcol = scol2;
  if (ecol > maxcol) maxcol = ecol;
  xdbg<<"maxcol = "<<maxcol<<endl;

  vector<PotentialStar*> maybestars;
  double x=0,y=0,m=0,size=0,s2=0;
  int e=0;
  string line;
  //MJ - new line
  long index=1;
  //END
  while (getline(fin,line)) {
    if (line[0] == '#') continue;
    // skip line if there are any *'s
    if (std::find(line.begin(),line.end(),'*')!=line.end()) continue;
    istringstream linein(line);
    // Read x,y,m,e,size,s2 from line
    string valstr;
    size_t i;
    for(i=1;i<=maxcol && linein>>valstr;i++) {
      char *end;
      const char *valcstr = valstr.c_str();
      bool err=false;
      if(i==xcol) {x = strtod(valcstr,&end); if (end==valcstr) err=true;}
      if(i==ycol) {y = strtod(valcstr,&end); if (end==valcstr) err=true;}
      if(i==mcol) {m = strtod(valcstr,&end); if (end==valcstr) err=true;}
      if(i==ecol) {e = strtol(valcstr,&end,10); if (end==valcstr) err=true;}
      if(i==scol1) {size = strtod(valcstr,&end); if (end==valcstr) err=true;}
      if(i==scol2) {s2 = strtod(valcstr,&end); if (end==valcstr) err=true;}
      if (err) {
        dbg<<"Error reading line: \n"<<line<<endl;
        myerror("Reading catalog");
      }
    }
    if (i<=maxcol) {
      dbg<<"Not enough columns in line: \n"<<line;
      myerror("Not enough columns in line.  \nThe specified column for one of x,y,m,etc. is too large");
    }
    // If there is an errcode to check, do so.
    if (ecol>0 && (e & ~okerrcode)) continue;

    if (scol2>0) size += s2;
    if (size < minsize) continue;
    if (size > maxsize) continue;
    if (m < minmag) continue;
    if (m > maxmag) continue;
    xdbg<<"position= "<<x<<','<<y<<", size = "<<size<<", err = "<<e<<endl;
    if (!logsize) size = log(size);
    //MJ - Changed line
    maybestars.push_back(new PotentialStar(Position(x,y),m,size,index,line));
    //END
  }
  dbg<<"added "<<maybestars.size()<<" objects\n";
  
  vector<PotentialStar*> stars = FindStars(maybestars);

  ofstream fout(outfile.c_str());
  dbg<<"opened outfile: "<<outfile<<endl;
  if (!fout) myerror("opening outfile");
  for(size_t i=0;i<stars.size();i++) if(stars[i]->GetMag()<=maxoutmag) {
    //MJ 
    //Here, you can access the indices using stars[i].GetIndex()
    //and do whatever you want with that, rather than writing out 
    //the original line string as I did.
    dbg<<stars[i]->GetLine()<<endl;
    fout<<stars[i]->GetLine()<<endl;
  }

  if (dbgout && dbgout != &cout) delete dbgout;
  return 0;
}


bool ReadParamFile(const vector<string>& paramsrootnames,
  const string& suffix, bool def=false)
{
  size_t i=0;
  bool foundgood;
  do {
    dbg<<"trying: "<<paramsrootnames[i]+suffix<<endl;
    ifstream paramin((paramsrootnames[i] + suffix).c_str());
    foundgood = bool(paramin);
  } while (!foundgood && ++i<paramsrootnames.size());
  if (!foundgood) {
    if (def) return true;
    else myerror("unable to find the specified parameter file");
  }
  ifstream paramin((paramsrootnames[i] + suffix).c_str());
  string line;
  while (getline(paramin,line)) {
    istringstream linein(line);
    string param;
    linein >> param;
    if (!linein) continue;
    if (param[0] == '#') continue;
    if (param[0] == '!') continue;
    std::transform(param.begin(),param.end(),param.begin(),&tolower);
    if(param=="minsize") linein >> minsize;
    else if(param=="maxsize") linein >> maxsize;
    else if(param=="minmag") linein >> minmag;
    else if(param=="maxmag") linein >> maxmag;
    else if(param=="maxoutmag") linein >> maxoutmag;
    else if(param=="ndivx") linein >> ndivx;
    else if(param=="ndivy") linein >> ndivy;
    else if(param=="startn1") linein >> startn1;
    else if(param=="startn1b") linein >> startn1b;
    else if(param=="magstep1") linein >> magstep1;
    else if(param=="reject1") linein >> reject1;
    else if(param=="startn2") linein >> startn2;
    else if(param=="magstep2") linein >> magstep2;
    else if(param=="minbinsize") linein >> minbinsize;
    else if(param=="reject2") linein >> reject2;
    else if(param=="starsperbin") linein >> starsperbin;
    else if(param=="fitorder") linein >> fitorder;
    else if(param=="fitsigclip") linein >> fitsigclip;
    else if(param=="purityratio") linein >> purityratio;
    else if(param=="maxrefititer") linein >> maxrefititer;
    else if(param=="xcol") linein >> xcol;
    else if(param=="ycol") linein >> ycol;
    else if(param=="mcol") linein >> mcol;
    else if(param=="scol1") linein >> scol1;
    else if(param=="scol2") linein >> scol2;
    else if(param=="ecol") linein >> ecol;
    else if(param=="okerrcode") linein >> okerrcode;
    else if(param=="baderrcode") 
      { int badec; linein >> badec; okerrcode = ~badec; }
    else if(param=="logsize") linein >> logsize;
    else if(param=="maxrms") linein >> maxrms;
    else if(param=="binsize1") linein >> binsize1;
    else if(param=="maxratio1") linein >> maxratio1;
    else if(param=="okvalcount") linein >> okvalcount;
    else if(param=="miniter1") linein >> miniter1;
    else if(param=="miniter2") linein >> miniter2;
    else {
      cerr << "Invalid parameter: "<<param<<endl;
      return false;
    }
  }
  return true;
}
 
 
#include "RequireNext.h"

void ReadCmdLine(int argc, char **argv,
    string *infile,string *outfile)
{
  bool err=0;
  string tempstring;

  // paramsrootnames contains a list of places to look for the
  // parameter files.  Start with the current directory and the
  // specified location for the executable.  Then add the 
  // rest of the path.
  vector<string> paramsrootnames(1,"./findstars.params.");
  paramsrootnames.push_back((string) argv[0] + ".params.");

  string path = std::getenv("PATH");
  xdbg<<"path = "<<path<<endl;
  // This assumes path is of form :/dir1:/dir2:/dir3:...:dirlast
  size_t start = 1;
  while (start != path.npos) {
    size_t end = path.find(':',start);
    string dir = path.substr(start,end-start);
    xdbg<<"extracting dir: "<<dir<<endl;
    paramsrootnames.push_back(dir+"/findstars.params.");
    start = end;
    if (start != path.npos) start++;
  }
  
  if(argc<3) err=true;
  else {
    *infile = argv[1];
    *outfile = argv[2];
  }

  ReadParamFile(paramsrootnames,"default",true);

  char *end;
  for(int i=3;i<argc && !err;i++) {
    if (argv[i][0] == '-') {
      switch(argv[i][1]) {
      case 'd':
	if (argv[i][2] == 'x') XDEBUG=true;
	require_next(dbgout=&cout) {
	  if (!(dbgout=new ofstream(argv[++i]))) err=true;
        }
        dbgout->setf(std::ios_base::unitbuf);
        break;
      case 'p':
        require_next(err=true) {
          if (!ReadParamFile(paramsrootnames,argv[++i])) {
            err=true;
            cerr<<"Error reading specified parameter file\n";
          }
        }
        break;
      case 'f':
        if (argv[i][2] == 'o') {
          require_next(err=true) {
            fitorder=strtol(argv[++i],&end,10);
            if (end == argv[i]) err=true;
          }
        }
        else if (argv[i][2] == 's') {
          require_next(err=true) {
            fitsigclip=strtod(argv[++i],&end);
            if (end == argv[i]) err=true;
          }
        }
        else err=true;
        break;
      case 'c':
        switch(argv[i][2]) {
          case 'e':
            require_next(err=true) {
              ecol = strtol(argv[++i],&end,10);
              if (end == argv[i]) err=true;
            }
            break;
          case 'x':
            require_next(err=true) {
              xcol = strtol(argv[++i],&end,10);
              if (end == argv[i]) err=true;
            }
            break;
          case 'y':
            require_next(err=true) {
              ycol = strtol(argv[++i],&end,10);
              if (end == argv[i]) err=true;
            }
            break;
          case 'm':
            require_next(err=true) {
              mcol = strtol(argv[++i],&end,10);
              if (end == argv[i]) err=true;
            }
            break;
          case 's':
            require_next(err=true) {
              scol1 = strtol(argv[++i],&end,10);
              if (end == argv[i]) err=true;
              require_next(scol2=0) {
                scol2 = strtol(argv[++i],&end,10);
                if (end == argv[i]) err=true;
              }
            }
            break;
          default:
            err=true;
        }
        break;
      case 'e':
        if (argv[i][2] == 'o') {
          if (argv[i][3] != 'k') err=true;
          else require_next(err=true) {
            okerrcode = strtol(argv[++i],&end,10);
            if (end == argv[i]) err=true;
          }
        }
        else if (argv[i][2] == 'b') {
          if (argv[i][3] != 'a') err=true;
          else if (argv[i][4] != 'd') err=true;
          else require_next(err=true) {
            okerrcode = strtol(argv[++i],&end,10);
            if (end == argv[i]) err=true;
            else okerrcode = ~okerrcode;
          }
        }
        else err=true;
        break;
      case 'r':
        require_next(err=true) {
          purityratio = strtod(argv[++i],&end);
          if (end == argv[i]) err=true;
        }
        break;
      default:
        err=true;
        break;
      }
    }
  }

  if(err) {
    cerr<<
"Usage: findstars infile outfile [-p paramfile] [-fo fitorder] \n\
    [-fs fitsigclip] [-r purityratio] [-d[x] [debugfile]] \n\
The first parameter is the raw catalog \n\
The second parameter is the output file with the stars \n\
The default input format is: \n\
    * x y m * * ixx iyy \n\
Flags: (which must come after the two file names) \n\
    -p Read algorithm parameters from a specified parameter file \n\
       Note: only specify the ending.  \"-p btc\" opens the parameter \n\
       file called findstars.params.btc \n\
       The program looks for parameter files in the current directory \n\
       first, then in the directory specified for the executable, \n\
       then the rest of your path \n\
    -fo Define the order of the fit of size = f(x,y) \n\
       eg. a second order fit is f(x,y) = a+bx+cy+dx^2+exy+fy^2 \n\
    -fs Define how many sigma to clip when fitting size = f(x,y) \n\
    -r Set the purity ratio.  If ratio = 0, the output will probably not  \n\
       include any galaxies.  As the ratio gets higher, you are more likely \n\
       to get a few interlopers.  0.1 is usually reasonable, giving only a  \n\
       few objects which might be galaxies. \n\
    -d Output debugging info to debugfile or (if omitted) to stdout \n\
       An x after the d includes even more debugging info \n";
    exit(1);
  }

  dbg<<"Using the following parameters:\n";
  dbg<<"minsize = "<<minsize<<endl;
  dbg<<"maxsize = "<<maxsize<<endl;
  dbg<<"minmag = "<<minmag<<endl;
  dbg<<"maxmag = "<<maxmag<<endl;
  dbg<<"ndivx = "<<ndivx<<endl;
  dbg<<"ndivy = "<<ndivy<<endl;
  dbg<<"startn1 = "<<startn1<<endl;
  dbg<<"startn1b = "<<startn1b<<endl;
  dbg<<"magstep1 = "<<magstep1<<endl;
  dbg<<"reject1 = "<<reject1<<endl;
  dbg<<"startn2 = "<<startn2<<endl;
  dbg<<"magstep2 = "<<magstep2<<endl;
  dbg<<"minbinsize = "<<minbinsize<<endl;
  dbg<<"reject2 = "<<reject2<<endl;
  dbg<<"starsperbin = "<<starsperbin<<endl;
  dbg<<"fitorder = "<<fitorder<<endl;
  dbg<<"fitsigclip = "<<fitsigclip<<endl;
  dbg<<"purityratio = "<<purityratio<<endl;
  dbg<<"maxrefititer = "<<maxrefititer<<endl;
  dbg<<"xcol = "<<xcol<<endl;
  dbg<<"ycol = "<<ycol<<endl;
  dbg<<"mcol = "<<mcol<<endl;
  dbg<<"scol1 = "<<scol1<<endl;
  dbg<<"scol2 = "<<scol2<<endl;
  dbg<<"ecol = "<<ecol<<endl;
  dbg<<"okerrcode = "<<okerrcode<<endl;
  dbg<<"(baderrcode = ~okerrcode = "<<~okerrcode<<")\n";
  dbg<<"logsize = "<<logsize<<endl;
  dbg<<"maxrms = "<<maxrms<<endl;
  dbg<<"binsize1 = "<<binsize1<<endl;
  dbg<<"maxratio1 = "<<maxratio1<<endl;
  dbg<<"okvalcount = "<<okvalcount<<endl;
  dbg<<"miniter1 = "<<miniter1<<endl;
  dbg<<"miniter2 = "<<miniter2<<endl;
}

