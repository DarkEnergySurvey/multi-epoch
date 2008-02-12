#include "FindStarsLib.h"


vector<PotentialStar*> 
FindStars(vector<PotentialStar*>& allobj)
{
  dbg<<"starting FindStars, allobj.size() = "<<allobj.size()<<endl;
  if (startn2 > allobj.size()) 
    myerror("Lower startn2.  There aren't that many objects in catalog.\n");

  // sort allobj by magnitude
  std::sort(allobj.begin(),allobj.end(),std::mem_fun(&PotentialStar::MagCompare));
  dbg<<"sorted allobj\n";

// First stage:
// Split area into sections
// For each secion find as many stars as possible.
// Use these stars to fit the variation in rsq across the image.

  // Make bounds of whole region called totalbounds
  Bounds totalbounds;
  for(size_t k=0;k<allobj.size();k++) totalbounds += allobj[k]->GetPos();
  dbg<<"totalbounds = \n"<<totalbounds<<endl;

  // boundsar is the 3x3 (ndivx x ndivy) array of quadrant bounds
  // call it qbounds even though it won't be quadrants unless 2x2
  vector<vector<Bounds> > qbounds = totalbounds.Divide(ndivx,ndivy);
  xdbg<<"made qbounds\n";

  // probstars will be our first pass list of probable stars
  vector<PotentialStar*> probstars;

  // For each section, find the stars and add to probstars
  for(size_t i=0;i<ndivx;i++) for(size_t j=0;j<ndivy;j++) {
    dbg<<"i,j = "<<i<<','<<j<<": bounds = "<<qbounds[i][j]<<endl;

    // someobj are the objects in this section
    // Note that someobj is automatically sorted by magnitude, since
    // allobj was sorted.
    vector<PotentialStar*> someobj;
    for(size_t k=0;k<allobj.size();k++) 
      if (qbounds[i][j].Includes(allobj[k]->GetPos())) 
        someobj.push_back(allobj[k]);
    dbg<<"added "<<someobj.size()<<" obj\n";

    // Check to make sure there are enough objects in section.
    // If not, then ndiv are too high.  Lower them.
    if (someobj.size() < startn1) {
      if (ndivx == 1 && ndivy == 1) 
        myerror("Lower startn1.  There aren't that many objects in catalog.\n");
      if (ndivx > 1) ndivx--;
      if (ndivy > 1) ndivy--;
      starsperbin = (int) ((MAX(ndivx,ndivy)+1.)/MAX(ndivx,ndivy)*starsperbin);
      dbg<<"Warning: Reducing ndivx,y to "<<ndivx<<','<<ndivy<<endl;
      dbg<<"new starsperbin = "<<starsperbin<<endl;
      cerr<<"Warning: Reducing ndivx,y to "<<ndivx<<','<<ndivy<<endl;
      return FindStars(allobj);
    }

    // Does a really quick and dirty fit to the bright stars
    // Basically it takes the 10 smallest of the 50 brightest objects,
    // finds the peakiest 5, then fits their sizes to a 1st order function.
    // It also gives us a rough value for the sigma
    Legendre2D<double> linearf(qbounds[i][j]);
    double sigma;
    RoughlyFitBrightStars(someobj,&linearf,&sigma);
    dbg<<"fit bright stars: sigma = "<<sigma<<endl;

    // Calculate the min and max values of the (adjusted) sizes
    double minsize,maxsize;
    FindMinMax(someobj,&minsize,&maxsize,linearf);
    dbg<<"min,max = "<<minsize<<','<<maxsize<<endl;

    // Find the objects clustered around the stellar peak.
    vector<PotentialStar*> qpeaklist =
      GetPeakList(someobj,binsize1,minsize,maxsize,
        startn1,miniter1,magstep1,maxratio1,true,linearf);
    dbg<<"peaklist has "<<qpeaklist.size()<<" stars\n";

    // Remove outliers using a median,percentile rejection scheme.
    // binsize1/2. is the minimum value of "sigma".
    OutlierReject(qpeaklist,reject1,binsize1/2.,linearf);
    dbg<<"rejected outliers, now have "<<qpeaklist.size()<<" stars\n";

    // Use at most 10 (starsperbin) stars per region to prevent one region 
    // from dominating the fit.  Use the 10 brightest stars to prevent being
    // position biased (as one would if it were based on size
    if (qpeaklist.size() < starsperbin) {
      dbg<<"Warning: only "<<qpeaklist.size()<<" stars found in section ";
      dbg<<i<<','<<j<<endl;
      probstars.insert(probstars.end(),qpeaklist.begin(),qpeaklist.end());
    }
    else {
      std::sort(qpeaklist.begin(),qpeaklist.end(),
	  std::mem_fun(&PotentialStar::MagCompare));
      probstars.insert(probstars.end(),qpeaklist.begin(),
          qpeaklist.begin()+starsperbin);
    }
    dbg<<"added to probstars\n";
  } // for i,j
  xdbg<<"done ij loop\n";
  size_t nstars = probstars.size();
  dbg<<"nstars = "<<nstars<<endl;

  // Now we have a first estimate of which objects are stars.
  // Fit a quadratic function to them to characterize the variation in size
  Legendre2D<double> f(totalbounds);
  double sigma;
  FitStellarSizes(&f,fitorder,fitsigclip,probstars,&sigma);

// Second stage:
// Use the fitted function for the size we just got to adjust
// the measured sizes according to their position in the image.
// Make the histogram using measured - predicted sizes (still log
// size actually) which should then have the peak very close to 0.
// Set the bin size for this new histogram to be the rms scatter of the
// stellar peak from pass 1.
// This time step by 0.25 mag (magstep2).

  // Find the values of minsize,maxsize for the whole thing with the new f
  double minsize,maxsize;
  FindMinMax(allobj,&minsize,&maxsize,f);
  dbg<<"new minmax = "<<minsize<<','<<maxsize<<endl;

  // Find the objects clustered around the stellar peak.
  // Note that the binsize is a set fraction of sigma (0.5), so this
  // should make the stellar peak clearer.
  // Also, the f at the end means the functional fitted size will be
  // subtracted off before adding to the histogram.
  probstars = GetPeakList(allobj,0.5*sigma,minsize,maxsize,
        startn2,miniter2,magstep2,purityratio,false,f);
  dbg<<"probstars has "<<probstars.size()<<" stars\n";

  // Remove outliers using a median,percentile rejection scheme.
  OutlierReject(probstars,reject2,sigma,f);
  dbg<<"rejected outliers, now have "<<probstars.size()<<" stars\n";
  // Worth doing twice, since first pass usually throws out a lot of junk.
  OutlierReject(probstars,reject2,sigma,f);  
  dbg<<"rejected outliers, now have "<<probstars.size()<<" stars\n";

  // If you get a bad fit the first time through, it can take a 
  // few passes to fix it.
  // And always do at least one refit.
  bool refit = true;
  for(size_t iter=0;refit && iter<maxrefititer;iter++) {
    dbg<<"starting refit\n"<<endl;
    refit = false;
    vector<vector<vector<PotentialStar*> > > starsarray(ndivx,
	vector<vector<PotentialStar*> >(ndivy));

    // Add each star to the appropriate sublist
    for(size_t k=0;k<probstars.size();k++) 
      for(size_t i=0;i<ndivx;i++) for(size_t j=0;j<ndivy;j++)
        if(qbounds[i][j].Includes(probstars[k]->GetPos()))
          starsarray[i][j].push_back(probstars[k]);

    // Make fitlist, the new list of the 10 brightest stars per section
    vector<PotentialStar*> fitlist;
    for(size_t i=0;i<ndivx;i++) for(size_t j=0;j<ndivy;j++) {
      // if there are still < 10 stars, give a warning, and
      // just add all the stars to fitlist
      if (starsarray[i][j].size() < starsperbin) {
        dbg<<"Warning: only "<<starsarray[i][j].size()<<" stars in section ";
        dbg<<i<<','<<j<<endl;
        if (iter == maxrefititer-1) {
          cerr<<"Warning: only "<<starsarray[i][j].size()<<" stars in section ";
          cerr<<i<<','<<j<<endl;
        }
        fitlist.insert(fitlist.end(),starsarray[i][j].begin(),
          starsarray[i][j].end());
        refit = true;
      }
      else {
        // sort the sublist by magnitude
	std::sort(starsarray[i][j].begin(),starsarray[i][j].end(),
	    std::mem_fun(&PotentialStar::MagCompare));

        // add the brightest 10 to fitlist
        fitlist.insert(fitlist.end(),starsarray[i][j].begin(),
          starsarray[i][j].begin()+starsperbin);
      }
    }
    // Do all the same stuff as before (refit f, get new min,max,
    // find the peaklist again, and reject the outliers)
    nstars = fitlist.size();
    FitStellarSizes(&f,fitorder,fitsigclip,fitlist,&sigma);
    FindMinMax(allobj,&minsize,&maxsize,f);
    dbg<<"new minmax = "<<minsize<<','<<maxsize<<endl;
    probstars = GetPeakList(allobj,0.5*sigma,minsize,maxsize,
        startn2,miniter2,magstep2,purityratio,false,f);
    dbg<<"probstars has "<<probstars.size()<<" stars\n";
    OutlierReject(probstars,reject2,sigma,f);
    OutlierReject(probstars,reject2,sigma,f);
    dbg<<"rejected outliers, now have "<<probstars.size()<<" stars\n";
    if (probstars.size() < nstars) {
      refit = true;
      dbg<<"fewer than "<<nstars<<" - so refit\n";
    }
  } // while refit (and iter < maxrefititer)

  dbg<<"done FindStars\n";

  for(size_t i=0;i<probstars.size();i++) {
    xdbg<<"stars["<<i<<"]: size = "<<probstars[i]->GetSize();
    xdbg<<", size - f(pos) = ";
    xdbg<<probstars[i]->GetSize()-f(probstars[i]->GetPos())<<endl;
    xdbg<<probstars[i]->GetLine()<<endl;
  }
  return probstars;
}

void FindMinMax(const vector<PotentialStar*>& list, 
    double *min, double *max, const Function2D<double>& f)
{
  double min1,max1;
  min1 = max1 = list[0]->GetSize()-f(list[0]->GetPos());
  for(size_t k=1;k<list.size();k++) {
    double size = list[k]->GetSize() - f(list[k]->GetPos());
    if (size > max1) max1 = size;
    if (size < min1) min1 = size;
  }
  *min = min1;
  *max = max1;
}

class CompareDistanceTo {
  const vector<PotentialStar*>& list;
  const Position& comp;
public:
  CompareDistanceTo(const vector<PotentialStar*>& l,
    const PotentialStar* c) : list(l),comp(c->GetPos()) {}
  bool operator()(PotentialStar* s1, PotentialStar* s2) const
    { return (norm(s1->GetPos()-comp) < norm(s2->GetPos()-comp)); }
};

void OutlierReject(vector<PotentialStar*>& list, 
    double nsigma, double minsigma, const Function2D<double>& f)
// This rejects outliers by finding the median and the quartile 
// values, and calls sigma the average of the two quartile deviations.
// "nsigma" is then how many of these "sigma" away from the median
//  to consider something an outlier.
{
  if (list.size() <= 4) return;
  // Find median, 1st and 3rd quartile stars;
  vector<PotentialStar*> modiflist(list.size());
  for(size_t k=0;k<list.size();k++) {
    modiflist[k] = new PotentialStar(*list[k]);
    double newsize = list[k]->GetSize() - f(list[k]->GetPos());
    modiflist[k]->SetSize(newsize);
  }
  std::sort(modiflist.begin(),modiflist.end(),
      std::mem_fun(&PotentialStar::SizeCompare));
  PotentialStar* mstar = modiflist[modiflist.size()/2];
  PotentialStar* q1star = modiflist[modiflist.size()/4];
  PotentialStar* q3star = modiflist[modiflist.size()*3/4];
  double median = mstar->GetSize();
  double q1 = q1star->GetSize();
  double q3 = q3star->GetSize();
  double sigma = MAX(q3-median,median-q1);
  sigma = MAX(sigma,minsigma);
//  double sigma = MAX((q3-q1)/2.,minsigma);
  xdbg<<"q1,m,q3 = "<<q1<<" , "<<median<<" , "<<q3<<endl;
  xdbg<<"sigma = "<<sigma<<", nsig * sigma = "<<nsigma*sigma<<endl;
  // Remove elements which abs(x->GetSize()-median) > nsigma*sigma
  size_t j=0;
  for(size_t k=0; k<modiflist.size();k++) {
    if (abs(modiflist[k]->GetSize()-median)>nsigma*sigma) {
      xdbg<<"k = "<<k<<", size = "<<modiflist[k]->GetSize();
      xdbg<<" might be an outlier (median = "<<median<<")\n";
    }
    list[j] = list[k];  // Note: update original list, not modified version.
    j++;
  }
  if (j<list.size()) list.erase(list.begin()+j,list.end());
  for(size_t k=0;k<modiflist.size();k++) delete modiflist[k];
}

vector<PotentialStar*> GetPeakList(const vector<PotentialStar*>& objlist,
  double binsize, double minsize, double maxsize,
  size_t startn, int miniter, double magstep, double maxsignifratio,
  bool firstpass,
  const Function2D<double>& f)
{
  if (binsize < minbinsize) binsize = minbinsize;

  // Make a histogram to find the stellar peak
  Histogram<PotentialStar*> hist(binsize,minsize,maxsize);

  // Start with the startn brightest objects, then step up in
  // magnitude by magstep at a time until histogram peak
  // gets dilluted.
  size_t k;
  Assert(startn<=objlist.size());
  for(k=0;k<startn;k++) 
    hist.Add(objlist[k]->GetSize()-f(objlist[k]->GetPos()),objlist[k]);
  xdbg<<"added "<<startn<<" objects\n";
  if(XDEBUG && dbgout) hist.Print(*dbgout,minsize,maxsize);

  // Get first estimates of the stellar and (first) galaxy peaks
  // along with the corresponding valleys.
  double peak1 = hist.FindFirstPeakAfter(minsize);
  xdbg<<"peak1 = "<<peak1<<" ("<<hist[peak1]<<")\n";
  double valley1 = hist.FindFirstValleyAfter(peak1);
  xdbg<<"valley1 = "<<valley1<<" ("<<hist[valley1]<<")\n";
  while (valley1 < 0.0) {
  // Since we subtract the fit, stars should be close to 0
    peak1 = hist.FindFirstPeakAfter(valley1);
    xdbg<<"new peak1 = "<<peak1<<" ("<<hist[peak1]<<")\n";
    valley1 = hist.FindFirstValleyAfter(peak1);
    xdbg<<"new valley1 = "<<valley1<<" ("<<hist[valley1]<<")\n";
  }
  double peak2 = hist.FindFirstPeakAfter(valley1);
  xdbg<<"peak2 = "<<peak2<<" ("<<hist[peak2]<<")\n";
  if (hist[peak2] == 0)
    myerror("Couldn't find a stellar peak - probably need to increase startn1 or\nlower startn1b.  There should be at least startn1b stars among the brightest \nstartn1 objects");
  // Sometimes the stellar peak is horned (eg. 212), in which case, find
  // the next peak
  double nbinsforhorn = firstpass ? 2.5 : 4.5;
  if (peak2-peak1 < nbinsforhorn*binsize) {
    valley1 = hist.FindFirstValleyAfter(peak2);
    peak2 = hist.FindFirstPeakAfter(peak2+binsize);
    xdbg<<"horn pattern: next peak2 = "<<peak2<<" ("<<hist[peak2]<<")\n";
  }
  double valley2 = hist.FindFirstValleyAfter(peak2);
  xdbg<<"valley2 = "<<valley2<<" ("<<hist[valley2]<<")\n";
  double valley0 = hist.FindFirstValleyBefore(peak1);
  if (!firstpass && valley0 > 0.) valley0 = hist.FindFirstValleyBefore(0.);
  xdbg<<"valley0 = "<<valley0<<" ("<<hist[valley0]<<")\n";

  // Get the stars in the first peak for the initial value of peaklist
  // When we're all done it will be the list of objects at the stellar peak.
  vector<PotentialStar*> peaklist=hist.GetRefsInRange(valley0,valley1);

  // Define the highest mag so far in list
  double highmag = objlist[startn-1]->GetMag();

  // Loop at least miniter times.  Done is set to false whenever
  // we've just found a new best peak.
  bool done=false;
  double prevvalley=valley1;
  double prevpeak=0.;
  for(int count = 0; count < miniter || !done; count++) {

    // Increment highmag
    highmag += magstep;
    xdbg<<"count = "<<count<<", highmag = "<<highmag<<endl;

    // Add new objects to histogram.
    for(;k<objlist.size() && objlist[k]->GetMag()<highmag;k++)
      hist.Add(objlist[k]->GetSize()-f(objlist[k]->GetPos()),objlist[k]);
    xdbg<<"hist has "<<k<<" objects\n";
    if(XDEBUG && dbgout) hist.Print(*dbgout,valley0,valley2);

    xdbg<<"valley0 = "<<valley0<<endl;
    // Find the peak on the stellar side
    double peak = hist.FindFirstPeakAfter(valley0,!firstpass);
    valley0 = hist.FindFirstValleyBefore(peak); // for next pass
    xdbg<<"done findpeak: "<<peak<<endl;

    // Find the valley 
    // The !firstpass allows poisson noise fluctuations for the final pass
    double valley = hist.FindFirstValleyAfter(peak,!firstpass);
    xdbg<<"done findvalley: "<<valley<<endl;

    // If valley ends up negative, find next peak and valley
    if (valley < 0.0) {
      double nextpeak = hist.FindFirstPeakAfter(valley,!firstpass);
      // if next peak is closer to 0 than current one, use new values.
      while (abs(nextpeak) < abs(peak)) {
	peak = nextpeak;
	xdbg<<"negative valley - new peak: "<<peak<<endl;
	valley = hist.FindFirstValleyAfter(peak,!firstpass);
	xdbg<<"new valley: "<<valley<<endl;
	if (valley < 0.0)
	  nextpeak = hist.FindFirstPeakAfter(valley,!firstpass);
	else break;
      }
    }

    // For the first pass, be extra conservative and don't let the valley
    // position get much larger than a previous good value.
    // If the peak is also larger than prevvalley, something weird is
    // going on, so bail on this pass.
    if (firstpass && valley > prevvalley+binsize) {
      valley = prevvalley;
      xdbg<<"moved valley to "<<valley<<endl;
      if (peak >= valley) { done = true; continue; }
    }

    // If new peak is larger than the previous valley, and the new peak 
    // isn't closer to 0, then reject it (probably added enough objects to
    // the valley to fill it in completely).
    if (peak > prevvalley && abs(peak) > abs(prevpeak)) {
      xdbg<<"New peak passed previous valley.\n";
      done = true;
      continue;
    }
   
    // The refined counts allow the bins to be centered anywhere near
    // the respective peak and valley and finds how many objects
    // would fall in the optimally centered bin.
    size_t peakcount = hist.GetRefinedPeakCount(&peak);
    xdbg<<"peakcount = "<<peakcount<<" centered at "<<peak<<endl;
    xdbg<<"before refined: valley = "<<valley<<endl;
    size_t valleycount = hist.GetRefinedValleyCount(&valley);
    xdbg<<"valleycount = "<<valleycount<<" centered at "<<valley<<endl;

    // Check for the horned pattern and use the next valley if it is there.
    // But only if the new valley is at least as good as the first one.
    //double nbinsforhorn = firstpass ? 1.5 : 3.5;
    double nbinsforhorn = 1.5;
    if ((valley-peak < nbinsforhorn*binsize) && 
      (hist.FindFirstPeakAfter(valley) - valley) < nbinsforhorn*binsize) {
      double newvalley = hist.FindFirstValleyAfter(
        hist.FindFirstPeakAfter(valley));
      size_t newvalleycount = hist.GetRefinedValleyCount(&newvalley);
      if (((firstpass && newvalleycount <= valleycount) ||
           (!firstpass && valleycount > 0 && newvalleycount == 0))
            && newvalley <= prevvalley + nbinsforhorn*binsize) {
        xdbg<<"horn pattern detected.  new valley = "<<newvalley<<endl;
        xdbg<<"new valley count = "<<newvalleycount<<endl;
        valley = newvalley;
        valleycount = newvalleycount;
      }
    }

    // The significance of the peak is the ratio of the number in the
    // valley to the number in the peak
    // So _small_ significances are good.
    // Has to be this way, since valleycount can be 0,
    // so peakcount/valleycount can be undefined.
    double signif = (double)valleycount/(double)peakcount;
    xdbg<<"signif = "<<signif<<endl;

    // If the valley count is <= okvalcount then drop the significance
    // to 0 regardless of the peak count, since sometimes the peak is fairly
    // broad so peakcount isn't high enough to get a good ratio.
    if (firstpass && valleycount <= okvalcount) {
      signif = 0.;
      xdbg<<"reset signif to 0, since valleycount "<<valleycount<<" <= ";
      xdbg<<okvalcount<<endl;
    }

    // If we have a new good peak, get all the stars in it for peaklist
    // Also make sure we repeat the loop by setting done = false
    if (signif <= maxsignifratio) {
      xdbg<<"good signif value\n";
      // Range is symmetrical about peakvalue with upper limit at valley
      double peakstart = MIN(2.*peak-valley,peak-binsize*0.5);
      vector<PotentialStar*> temp = 
        hist.GetRefsInRange(peakstart,valley);
      xdbg<<"get refs from "<<peakstart<<" to "<<valley<<endl;
      xdbg<<"got refs: "<<temp.size()<<endl;
      // If this list has fewer objects than a previous list,
      // and it doesn't just have a tighter range,
      // don't take it.  Surprisingly, this is actually possible, and it
      // means something weird happened
      if (temp.size() >= peaklist.size() 
        || (valley<prevvalley && valley>prevpeak)
        || (peak > prevpeak) ) { 
        if (temp.size() >= peaklist.size()) {
          xdbg<<"tempsize = "<<temp.size()<<" >= "<<peaklist.size();
        }
        else if ((valley<prevvalley && valley>prevpeak)) {
          xdbg<<"valley = "<<valley<<" < "<<prevvalley;
        }
        else {
          xdbg<<"peak = "<<peak<<" > "<<prevpeak;
        }
        xdbg<<" so keep adding...\n";
        peaklist = temp;
        done = false;
        prevvalley = valley;
        prevpeak = peak;
      }
      else {
        xdbg<<"tempsize < "<<peaklist.size();
        xdbg<<" and peak,valley didn't contract, so stop adding now.\n";
        done=true;
      }
    }
    else {
      xdbg<<"signif = "<<signif<<" > "<<maxsignifratio<<endl;
      done=true; // maybe.  still loop if count < miniter
    }

    // If we've already used all the stars, don't loop anymore
    if (k == objlist.size()) {
      xdbg<<"no more to add\n";
      count = miniter; done = true; 
    }
  } // for count
  xdbg<<"done finding peaklist\n";
  return peaklist;
}

void FitStellarSizes(Function2D<double> *f, size_t order, double sigclip,
    const vector<PotentialStar*>& starlist, double *outsigma)
{
  // Make list of positions
  vector<Position> poslist(starlist.size());
  std::transform(starlist.begin(),starlist.end(),poslist.begin(),
      std::mem_fun(&PotentialStar::GetPos));

  // Make list of sizes
  vector<double> sizelist(starlist.size());
  std::transform(starlist.begin(),starlist.end(),sizelist.begin(),
      std::mem_fun(&PotentialStar::GetSize));

  // Use all stars in list (to start with anyway), so all true
  vector<bool> uselist(starlist.size(),true);

  if (starlist.size() <= (order+1)*(order+2)/2)
    myerror("Not enough stars to do fit.\nIncrease starsperbin or decrease fitorder");

  // Do the fit.
  double chisq;
  int dof;
  xdbg<<"before outlier fit\n";
  f->OutlierFit(order,sigclip,poslist,sizelist,&uselist,0,&chisq,&dof,0);
  xdbg<<"after outlier fit\n";
  //size_t n = count(uselist.begin(),uselist.end(),true);

  xdbg<<"chisq,dof,sigma = "<<chisq<<','<<dof<<','<<sqrt(chisq/dof)<<endl;
  *outsigma = sqrt(chisq/dof);
}

void RoughlyFitBrightStars(const vector<PotentialStar*>& objlist,
    Function2D<double> *f,double *outsigma)
{
  Assert(startn1 <= objlist.size()); // This is checked in FindStars
  // objlist is already sorted by magnitude
  // make a new list with just the 50 brightest objects:
  vector<PotentialStar*> brightlist(objlist.begin(),
    objlist.begin()+startn1);
  
  // now sort this list by size
  std::sort(brightlist.begin(),brightlist.end(),
      std::mem_fun(&PotentialStar::SizeCompare));
 
  xdbg<<"brightlist is:\n";
  for(size_t i=0; i<std::min(size_t(20),startn1); i++)
    xdbg<<brightlist[i]->GetMag()<<" , "<<brightlist[i]->GetSize()<<endl;
  // Of the smallest 10, find the 5 that form the tightest peak
  Assert(brightlist.size() == startn1);
  if (2*startn1b-1>=startn1) 
    myerror("Decrease startn1b.  It must be less than 1/2 startn1.");
  Assert(2*startn1b-1<brightlist.size());
  size_t five = startn1b;
  size_t peakstart = 0;
  double peakwidth = brightlist[five-1]->GetSize()-brightlist[0]->GetSize();
  for(size_t k=1;k<=five;k++) {
    Assert(k+five-1<brightlist.size());
    if (brightlist[k+five-1]->GetSize()-brightlist[k]->GetSize() < peakwidth) {
      peakwidth = brightlist[k+five-1]->GetSize()-brightlist[k]->GetSize();
      peakstart = k;
    }
  }
  xdbg<<"found peak starting at "<<peakstart<<" with width "<<peakwidth<<endl;

/*
  f->SetTo(brightlist[peakstart]->GetSize());
*/

  size_t k1=peakstart, k2=peakstart+five;
  // Now expand list to be 2*binsize1 wide
  while (k1 > 0 && brightlist[peakstart+2]->GetSize() -
        brightlist[k1-1]->GetSize() < binsize1) k1--;
  while (k2 < brightlist.size() && brightlist[k2]->GetSize() - 
        brightlist[k1]->GetSize() < 2.*binsize1) k2++;
  xdbg<<"expanded to "<<k1<<','<<k2<<endl;
  
  // Call these objects in the expanded peak stars
  vector<PotentialStar*> starlist(brightlist.begin()+k1,
        brightlist.begin()+k2);
 
  // Fit f using a linear fit with 2 sigma clipping
  FitStellarSizes(f,0,2.,starlist,outsigma);

  // if sigma is too big, drop either the first or last "star" and try again.
  while(*outsigma > maxrms && starlist.size() > 4) {
    if (abs(starlist.front()->GetSize()-(*f)(starlist.front()->GetPos()))
      > abs(starlist.back()->GetSize()-(*f)(starlist.back()->GetPos()))) {
      starlist.erase(starlist.begin());
      xdbg<<"Erased first star.  New size = "<<starlist.size()<<endl;
    }
    else {
      starlist.erase(starlist.end()-1);
      xdbg<<"Erased last star.  New size = "<<starlist.size()<<endl;
    }
    FitStellarSizes(f,0,2.,starlist,outsigma);
  }
}

