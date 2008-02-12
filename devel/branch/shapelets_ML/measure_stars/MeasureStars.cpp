#include "MeasureStars.hpp"


MeasureStars::MeasureStars() {}
//MeasureStars::MeasureStars(string config_file) 
MeasureStars::MeasureStars(
        string msconf,
        string fsconf, 
        string imfile,
        string catfile,
        string allout,
        string starout)
{
  xdbg<<"Start MeasureStars constructor\n";

    ReadConfig(msconf);

    xdbg<<"after read config\n";

    mConfig["findstars_config"] = fsconf;

    mConfig["image_file"] = imfile;
    mConfig["cat_file"] = catfile;
    mConfig["all_output_file"] = allout;
    mConfig["star_output_file"] = starout;

    ReadCat();
    xdbg<<"after read cat\n";
    FixCat();
    TestCat();
    xdbg<<"after test cat\n";
    ReadImage( mConfig["image_file"], mConfig["image_hdu"]);
    xdbg<<"after read image\n";
    TestImage();
    xdbg<<"after test image\n";
    SetParameters();
    xdbg<<"after setparams\n";

    // First pass: Get sizes of objects for s-g separation
    ProcessCat(1);
    xdbg<<"after processcat 1\n";
    // Get a star list
    //XDEBUG = true;
    FindStars();
    //XDEBUG = false;
    xdbg<<"after findstars\n";
    // Second pass: Now process the stars and output shapelet coefficients
    ProcessCat(2);
    xdbg<<"after processcat 2\n";

}
MeasureStars::~MeasureStars() {}

void MeasureStars::ReadConfig(string file)
{

    std::vector<string> reqfields = RequiredConfigFields(); 
  

    cout<<"Loading config file: "<<file<<endl;
    mConfig.Load(file);

    // Make sure the required fields are present
    std::vector<string>::iterator it;
    for (it=reqfields.begin(); it!=reqfields.end(); ++it)
    {
        if (! mConfig.keyExists(*it) )
        {
            cout<<"MeasureStars config does not contain required field: \""
                <<*it<<"\""<<endl;
            ErrorExit("",READERROR);
        }
    }

    dbg << mConfig;


    if (mConfig["sky_type"] == mConfig["c_global_sky"])
        mUseGlobalSky=true;
    else
        mUseGlobalSky=false;
    if (mConfig["skyvar_type"] == mConfig["c_global_skyvar"])
        mUseGlobalSkyVar=true;
    else
        mUseGlobalSkyVar=false;

}

std::vector<string> MeasureStars::RequiredConfigFields()
{
  std::vector<string> reqfields;

  reqfields.push_back("image_hdu");
  reqfields.push_back("cat_hdu");
  reqfields.push_back("defval");
  reqfields.push_back("boxsize_pass1");
  reqfields.push_back("boxsize_pass2");
  reqfields.push_back("firstpass_order");
  reqfields.push_back("secondpass_order");

  return(reqfields);  
}

void MeasureStars::FindStars()
{
    // This where we will select stars.
    StarFinder sf( mConfig["findstars_config"] );
    mStarList = sf.ProcessSEList(
            mCat.sh_flags,
            mCat.x0,
            mCat.y0,
            mCat.sh_sigma1,  // sigma1 is first-pass sigma
            mCat.mag_auto);
    for (unsigned long i=0;i<mStarList.size();i++)
    {
        unsigned long ii = mStarList[i];
        dbg<<"  "<<ii<<"\n";
        mCat.sh_flags[ii] |= SH_STAR;
    }
}


void MeasureStars::ProcessCat(int pass)
{
    int ntot;
    int keep;

    double sigma;

    ofstream ofs;

    int testout=1;
    size_t order;

    cout<<"Processing catalog: pass = "<<pass<<endl;
    if (pass == 1)
    {
        // First pass
        ntot = mCat.x0.size();

        string ofile=mConfig["all_output_file"];
        cout<<"  Writing all to file: "<<ofile<<"\n";
        ofs.open(ofile.c_str(),std::ios::out);
        params.SetMaxSize(mConfig["boxsize_pass1"]); 

        order = params.firstpassorder;
    }
    else if (pass==2)
    {
        // second pass
        ntot = mStarList.size();
        string ofile=mConfig["star_output_file"];
        cout<<"  Writing stars to file: "<<ofile<<endl;
        ofs.open(ofile.c_str(), std::ios::out);
        params.SetMaxSize(mConfig["boxsize_pass2"]); 

        order=params.outputorder;
    }
    else
    {
        ErrorExit("Currently only support pass = 1,2",FORMATERROR);
    }
 
    cout<<"  order = "<<order<<endl;

    int nuse = 0;
    int ngood = 0;
    unsigned long i;
    for (unsigned long ii=0; ii<ntot; ii++)
    {
        if (pass == 1)
        {
            // First pass
            i = ii;
            keep = (mCat.flags[i] == 0) ? 1 : 0;
        }
        else
        {
            // second pass
            i = mStarList[ii];
            keep = 1;
        }


        // Need to set default to -9999.0
        FullShape shape(order);
        //sigma = mConfig["defval"];
        if (keep)
        {
            nuse++;


            double x=mCat.x0[i]-1;
            double y=mCat.y0[i]-1;
            double pixscale=1;

            double ixx, ixy, iyy;
            if (pass == 1)
            {
                ixx=mCat.x2[i];
                ixy=mCat.xy[i];
                iyy=mCat.y2[i];
            }
            else
            {
                ixx=mCat.sh_sigma1[i]*mCat.sh_sigma1[i]/2.0;
                ixy=0.0;
                iyy=ixx;
            }

            double sky;
            if (mUseGlobalSky)
                sky = mGlobalSky;
            else
                sky = mCat.local_sky[i];

            xdbg<<
                "-------------------------------"<<endl<<
                "  index = "<<i<<endl<<
                "  id = "<<mCat.id[i]<<endl<<
                //"  s2n = "<<s2n<<endl<<
                "  x = "<<x<<endl<<
                "  y = "<<y<<endl<<
                "  pixscale = "<<pixscale<<endl<<
                "  ixx = "<<ixx<<endl<<
                "  ixy = "<<ixy<<endl<<
                "  iyy = "<<iyy<<endl<<
                //"  sz = "<<sz<<endl<<
                //"  ellip = "<<mCat.ellip[i]<<endl<<
                "  mag_auto = "<<mCat.mag_auto[i]<<endl<<
                "  sky = "<<sky<<endl<<
                "  flags = "<<mCat.flags[i]<<endl;

            Star star(mObjImage,x,y,pixscale,ixx,ixy,iyy,sky);


            int res=0;

            try 
            {
                res = star.MeasureShape(shape,true);
            }
            catch (std::exception)
            {
                cout<<"  Error measuring star.  Index = "<<i<<endl;
            }

            // These always get copied for now so we can diagnose failures
            mCat.sh_sigma[i] = shape.GetSigma();
            if (!res)
            {
                // Only one failure mode right now.
                if (pass==1)
                    mCat.sh_flags[i] |= SH_FIRSTPASS_FAILED;
                else
                    mCat.sh_flags[i] |= SH_SECONDPASS_FAILED;
            }
            else
            {
                ngood++;
                if (pass==1)
                {
                    // Will print later
                    mCat.sh_sigma1[i] = shape.GetSigma();
                }
                else
                {
                    // Only write star output for good ones 
                    ofs<<
                        mCat.id[i]<<","<<
                        mCat.mag_auto[i]<<","<<
                        mCat.sh_flags[i]<<","<<
                        shape.GetSigma()<<",";
                    shape.WriteCSV(ofs);
                    ofs<<endl;
                }
                   
            }
        }
        else
        {
            mCat.sh_flags[i] |= SH_NOMEAS;
        }

        // Always write output for first pass
        // For stars, will have already written output for
        // Good ones.
        if (pass == 1)
        {
            ofs<<
                mCat.id[i]<<","<<
                mCat.mag_auto[i]<<","<<
                mCat.sh_flags[i]<<","<<
                shape.GetSigma()<<",";
            shape.WriteCSV(ofs);
            ofs<<endl;
        }
    }

    cout<<"  Used "<<nuse<<"/"<<ntot<<" objects"<<
        " (Good "<<ngood<<")"<<endl;
    ofs.close();
}


void MeasureStars::ReadFromHeaders(string file)
{

    stringstream emessage;
    fitsfile *fptr;
    int fitserr=0;

    dbg<< "Reading first HDU from file " << file << endl;

    fits_open_file(&fptr,file.c_str(),READONLY,&fitserr);
    if (!fitserr==0) 
        ErrorExit("opening fits file",READERROR);

    int hdu=1, hdutype;
    printf("Moving to HDU: %d\n", hdu);
    fits_movabs_hdu(fptr, hdu, &hdutype, &fitserr);
    if (!fitserr==0)
    {
        fits_report_error(stderr, fitserr); /* print error report */
        emessage << "Error moving to extension " << hdu;
        ErrorExit(emessage.str().c_str(), READERROR);
    }


    char comment[40];
    fits_read_key_flt(fptr, "SEXTHLD", &mThreshold, comment, &fitserr);
    if (!fitserr==0)
    {
        fits_report_error(stderr, fitserr); /* print error report */
        ErrorExit("Error reading SEXTHLD card", READERROR);
    }  

    fits_close_file(fptr, &fitserr);
    if (!fitserr==0)
        ErrorExit("Error closing file",READERROR);


}


void MeasureStars::ReadCat()
{
    stringstream emessage;
    fitsfile *fptr;
    int fitserr=0;

    string file = mConfig["cat_file"];
    int    hdu  = mConfig["cat_hdu"];
    int    minrows = mConfig["minrows"];

    cout<< "Reading cat from file: " << file << endl;

    fits_open_file(&fptr,file.c_str(),READONLY,&fitserr);
    if (!fitserr==0) 
        ErrorExit("opening fits file",READERROR);

    int hdutype;

    dbg<<"Moving to HDU: "<<hdu<<endl;
    fits_movabs_hdu(fptr, hdu, &hdutype, &fitserr);
    if (!fitserr==0)
    {
        fits_report_error(stderr, fitserr); /* print error report */
        emessage << "Error moving to extension " << hdu;
        ErrorExit(emessage.str().c_str(), READERROR);
    }

    // These are not very portable.
    int nfound;
    long naxescat[2];
    long nrows;

    fits_read_keys_lng(fptr, "NAXIS", 1, 2, naxescat, &nfound, &fitserr);
    if (!fitserr==0)
    {
        fits_report_error(stderr, fitserr); /* print error report */
        ErrorExit("Error reading NAXIS card", READERROR);
    }
    nrows = naxescat[1];     
    dbg<<"naxes cat = "<<naxescat[0]<<" "<<naxescat[1]<<endl;
    cout<<"  nrows = "<<nrows<<endl;

    // Copy to class variable
    mNRows = nrows;

    if (nrows <= 0)
        ErrorExit("Rows must be >= 0", FORMATERROR);

    // MJ: <100 have basically no chance to find the stars
    if (nrows <= minrows)
      ErrorExit("Too few rows in catalog", FORMATERROR);

    // Allocate memory for the columns we will read
    ResizeCat(nrows);


    // Read each column in turn
    int colnum, anynull;
    long frow=1, felem=1, longnull=0;
    float floatnull=0;

    string name;

    // Number = id
    ReadFitsCol(fptr, "number", TLONG, (char *)&mCat.id[0]);
    ReadFitsCol(fptr, "xmin_image", TLONG, (char *)&mCat.i1[0]);
    ReadFitsCol(fptr, "xmax_image", TLONG, (char *)&mCat.i2[0]);
    ReadFitsCol(fptr, "ymin_image", TLONG, (char *)&mCat.j1[0]);
    ReadFitsCol(fptr, "ymax_image", TLONG, (char *)&mCat.j2[0]);

    ReadFitsCol(fptr, "x_image", TFLOAT, (char *)&mCat.x0[0]);
    ReadFitsCol(fptr, "y_image", TFLOAT, (char *)&mCat.y0[0]);

    ReadFitsCol(fptr, "x2_image", TFLOAT, (char *)&mCat.x2[0]);
    ReadFitsCol(fptr, "xy_image", TFLOAT, (char *)&mCat.xy[0]);
    ReadFitsCol(fptr, "y2_image", TFLOAT, (char *)&mCat.y2[0]);

    ReadFitsCol(fptr, "mag_auto", TFLOAT, (char *)&mCat.mag_auto[0]);
    ReadFitsCol(fptr, "magerr_auto", TFLOAT, (char *)&mCat.magerr_auto[0]);

    ReadFitsCol(fptr, "flags", TSHORT, (char *)&mCat.flags[0]);

    // Only create mem for local sky if used.
    if (mUseGlobalSky)
    {
        // This keyword is not currently there!
        if (0)
        {
            char comment[40];
            string skyname = mConfig["global_sky_name"];
            fits_read_key_flt(fptr, (char *)skyname.c_str(), &mGlobalSky, 
                    comment, &fitserr);
            if (!fitserr==0)
            {
                fits_report_error(stderr, fitserr); /* print error report */
                ErrorExit("Error reading sky value", READERROR);
            }  
        }
        else
        {
            mGlobalSky = mConfig["def_global_sky"];
        }
    }
    else
    {
        mCat.local_sky.resize(nrows);
        ReadFitsCol(fptr, "background", TFLOAT, (char *)&mCat.local_sky[0]);
    }





    fits_close_file(fptr, &fitserr);
    if (!fitserr==0)
        ErrorExit("Error closing file",READERROR);


}

/*
 * Make corrections to the catalog based on configurable parameters
 */
void MeasureStars::FixCat()
{
    float magoffset = mConfig["magoffset"];
    if (magoffset != 0)
    {
        for (int i=0;i<mCat.mag_auto.size();i++)
        {
            mCat.mag_auto[i] += magoffset;
        }
    }
}

void MeasureStars::ResizeCat(long nrows)
{
    mCat.id.resize(nrows);

    mCat.i1.resize(nrows);
    mCat.i2.resize(nrows);

    mCat.j1.resize(nrows);
    mCat.j2.resize(nrows);

    mCat.x0.resize(nrows);
    mCat.y0.resize(nrows);

    mCat.x2.resize(nrows);
    mCat.xy.resize(nrows);
    mCat.y2.resize(nrows);

    //mCat.a.resize(nrows);

    //mCat.ellip.resize(nrows);

    mCat.mag_auto.resize(nrows);
    mCat.magerr_auto.resize(nrows);

    mCat.flags.resize(nrows);

    // These must be initialized
    mCat.sh_sigma1.resize(nrows, -9999.0);
    mCat.sh_sigma.resize(nrows, -9999.0);
    mCat.sh_flags.resize(nrows,0);
}

#define SEG_HDU 3
void MeasureStars::ReadSeg(string file)
{

    stringstream emessage;
    fitsfile *fptr;
    int fitserr=0;

    cout<< "Reading segmentation image from file: " << file << endl;

    fits_open_file(&fptr,file.c_str(),READONLY,&fitserr);
    if (!fitserr==0) 
        ErrorExit("READERROR: Error opening fits file",READERROR);

    int hdutype;

    fits_movabs_hdu(fptr, SEG_HDU, &hdutype, &fitserr);
    if (!fitserr==0)
    {
        fits_report_error(stderr, fitserr); /* print error report */
        emessage << "Error moving to extension " << SEG_HDU;
        ErrorExit(emessage.str().c_str(), READERROR);
    }

    if (hdutype != IMAGE_HDU)
        ErrorExit("Extension is not an IMAGE_HDU", FORMATERROR);

    int bitpix, naxes;
    long sizes[2];

    cout<<"Getting image parameters\n";
    fits_get_img_param(fptr, int(2), &bitpix, &naxes, sizes, &fitserr);
    if (!fitserr==0) 
        ErrorExit("Error getting image parameters", READERROR);


    if (bitpix != SHORT_IMG) 
        ErrorExit("image is not short",FORMATERROR);
    if (naxes != 2) 
        ErrorExit("image is not 2d",FORMATERROR);

    long npix = sizes[0]*sizes[1];
    long fpixel[2] = {1,1};
    int anynul;
    mSegImage.resize(npix);

    cout<<"Reading ["<<sizes[0]<<"X"<<sizes[1]<<"]";
    fits_read_pix(fptr,
            TSHORT,
            fpixel,npix,0,
            (char *)&mSegImage[0],&anynul,
            &fitserr);
    if (!fitserr==0)
        ErrorExit("Error reading pixel data", READERROR);

    fits_close_file(fptr, &fitserr);
    if (!fitserr==0)
        ErrorExit("Error closing file",READERROR);


}

void MeasureStars::ReadImage(string file, int hdu)
{
    cout<< "Reading object image from file: " << file << endl;
    mObjImage.LoadFile(file.c_str(), hdu);
}

/*
   void MeasureStars::SetupNRand()
   {
// seed generator with #seconds since 1970
mRng.seed(static_cast<unsigned> (std::time(0)));
}
*/
/*
   void MeasureStars::FixSky(int xmin, 
   int xmax, 
   int ymin, 
   int ymax, 
   int id, 
   float sigsky)
   {

// select desired probability distribution
boost::normal_distribution<double> norm_dist(0.0, sigsky);

// bind random number generator to distribution, forming a function
boost::variate_generator<boost::mt19937&, boost::normal_distribution<double> >
normal_sampler(mRng, norm_dist);

if (xmin < 0 
|| xmax > mObjImage.GetMaxI()
|| ymin < 0
|| ymax > mObjImage.GetMaxJ())
{
ErrorExit("Rectangle out of range",RANGEERROR);
}

int nx = mObjImage.GetMaxI()+1;
int ny = mObjImage.GetMaxJ()+1;

// Fix all pixels in image that are exactly zero
for (int ix=xmin; ix < xmax; ix++)
if (ix > 0 && ix < nx)
{
for (int iy=ymin; iy < ymax; iy++)
if (iy > 0 && iy < ny)
{
//int ixiy = iy*mObjImage.GetMaxI() + ix;
int ixiy = iy*nx + ix;
//dbg<<"seg["<<ix<<","<<iy<<"] = "<<mSegImage[ixiy]<<endl;
if (mSegImage[ixiy] != id)
{
// Add some noise
double rnd = normal_sampler();
//dbg<<" before: "<<mObjImage(ix,iy);
//dbg<<" ["<<ix<<","<<iy<<"] Adding rand("<<sigsky<<") = "<<rnd<<endl;
mObjImage(ix,iy) += rnd;
}
}
}
}
*/






void MeasureStars::SetParameters()
{
    // Default is dudx = 1, dudy = 0, dvdx = 0, dvdy = 1
    params.dudx.reset(new Constant2D<double>(1.));
    params.dudy.reset(new Constant2D<double>(0.));
    params.dvdx.reset(new Constant2D<double>(0.));
    params.dvdy.reset(new Constant2D<double>(1.));

    params.firstpassorder=mConfig["firstpass_order"];
    params.outputorder=mConfig["secondpass_order"];
    params.fitforsky=false;
    params.minI = -1.e6;


    //params.roundnesstol = 1.e-5;
    //params.roundnesstol2 = 1.e-5;

}


void MeasureStars::TestCat()
{

    int ii = 225;
    dbg<<"Testing cat["<<ii<<"]"<<endl;
    dbg<<"  id = "<<mCat.id[ii]<<endl;

    dbg<<"  i1 = "<<mCat.i1[ii]<<endl;
    dbg<<"  i2 = "<<mCat.i2[ii]<<endl;
    dbg<<"  j1 = "<<mCat.j1[ii]<<endl;
    dbg<<"  j2 = "<<mCat.j2[ii]<<endl;

    dbg<<"  x0 = "<<mCat.x0[ii]<<endl;
    dbg<<"  y0 = "<<mCat.y0[ii]<<endl;

    dbg<<"  x2 = "<<mCat.x2[ii]<<endl;

    //dbg<<"  a = "<<mCat.a[ii]<<endl;
    //dbg<<"  threshold = "<<mCat.threshold[ii]<<endl;


}

void MeasureStars::TestImage()
{
    dbg<<"Testing image"<<endl;
    for (int ix=0; ix < mObjImage.GetMaxI(); ix++)
        for (int iy=0; iy < mObjImage.GetMaxJ(); iy++)
            if (mObjImage(ix,iy) > 0)
            {
                dbg<<
                    "  mObjImage("<<ix<<","<<iy<<") = "<<
                    mObjImage(ix,iy)<<endl;
                return;
            }
}

void MeasureStars::ReadFitsCol(
        fitsfile* fptr, 
        char* colname, 
        int coltype, 
        char* dptr)
{
    stringstream emessage;

    int fitserr=0;

    int anynull=0;
    long longnull=0;
    float floatnull=0;
    long frow=1, felem=1;
    float64 dblnull;

    int colnum;
    fits_get_colnum(fptr, CASEINSEN, colname, &colnum, &fitserr);
    if (!fitserr==0) {
        emessage<<"Error reading colnum for \""<<colname<<"\"";
        ErrorExit(emessage.str().c_str(), READERROR);
    }

    // using same dblnull everywhere probably OK.  Just needs to be big
    // enough to take the var I think.

    fits_read_col(fptr, coltype, colnum, frow, felem, mNRows, &dblnull, 
            dptr,
            &anynull, &fitserr);
    if (!fitserr==0) {
        emessage<<"Error reading column \""<<colname<<"\"";
        ErrorExit(emessage.str().c_str(), READERROR);
    }
    return;

}




