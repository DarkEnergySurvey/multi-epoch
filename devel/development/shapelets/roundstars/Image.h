//---------------------------------------------------------------------------
#ifndef ImageH
#define ImageH
//---------------------------------------------------------------------------

#include "Kernel.h"
#include "TMV.h"

class CImage;

class Image {

  public:

    Image();
    Image(const std::string& fitsfile, int ext=0); // Read new image from file

    Image(size_t xsize, size_t ysize) : // Create blank image
      xmin(0),xmax(xsize),ymin(0),ymax(ysize),
      sourcem(new tmv::Matrix<double,tmv::ColMajor>(xsize,ysize,0.)),
      itsm(new tmv::MatrixView<double>(sourcem->View())) {}

    Image(const Image& rhs) : // New copy of image
      xmin(rhs.xmin), xmax(rhs.xmax), ymin(rhs.ymin), ymax(rhs.ymax),
      sourcem(new tmv::Matrix<double,tmv::ColMajor>(*rhs.itsm)),
      itsm(new tmv::MatrixView<double>(sourcem->View())) {}

    Image(const Image& rhs, size_t x1, size_t x2, size_t y1, size_t y2) :
      // Subimage (with new storage)
      xmin(x1), xmax(x2), ymin(y1), ymax(y2),
      sourcem(new tmv::Matrix<double,tmv::ColMajor>(
	    rhs.itsm->SubMatrix(x1,x2,y1,y2))),
      itsm(new tmv::MatrixView<double>(sourcem->View())) {}

    ~Image() { if(sourcem) delete sourcem; delete itsm; }

    void LoadFile(const std::string& fitsfile, int ext=0); // Read new image from file

    void operator=(const Image& rhs) { *itsm = *rhs.itsm; }
    void Copy(const Image& rhs, size_t x1, size_t x2, size_t y1, size_t y2)
    { itsm->SubMatrix(x1,x2,y1,y2) = *rhs.itsm; }

    void Flush(const std::string& fitsfile); // Write back to file

    tmv::ConstMatrixView<double> GetM() const { return *itsm; }
    const tmv::MatrixView<double>& GetM() { return *itsm; }
    int GetXMin() const { return xmin; }
    int GetXMax() const { return xmax; }
    int GetYMin() const { return ymin; }
    int GetYMax() const { return ymax; }
    size_t GetMaxI() const { return itsm->colsize()-1; }
    size_t GetMaxJ() const { return itsm->rowsize()-1; }

    double& operator()(size_t i,size_t j) { return (*itsm)(i,j); }
    double operator()(size_t i,size_t j) const { return (*itsm)(i,j); }

    void operator+=(const Image& rhs) { *itsm += *rhs.itsm; }
    void operator*=(const FittedKernel& k);
    void operator=(const CImage& rhs);
    void operator+=(const CImage& rhs);

    void Clear() { itsm->Zero(); }

    bool IsSquare() { return itsm->IsSquare(); }

    Bounds GetBounds() const 
    { return Bounds(xmin,xmax,ymin,ymax); }

    Image SubImage(size_t x1, size_t x2, size_t y1, size_t y2)
      // SubImage refers to the same storage as this.
    { return Image(itsm->SubMatrix(x1,x2,y1,y2),x1,x2,y1,y2); }
      
    std::vector<std::vector<Image*> > Divide(size_t nx, size_t ny) const; 
    // SubImages refer to the same storage as this.

    double Interpolate(double x, double y) const;
    double QuadInterpolate(double x, double y) const;

  private:

    int xmin,xmax,ymin,ymax;
    tmv::Matrix<double,tmv::ColMajor>* sourcem;
    tmv::MatrixView<double>* itsm;

    Image(const tmv::MatrixView<double>& m, 
	size_t x1, size_t x2, size_t y1, size_t y2) :
      xmin(x1), xmax(x2), ymin(y1), ymax(y2),
      sourcem(0), itsm(new tmv::MatrixView<double>(m)) {}

};

#endif

