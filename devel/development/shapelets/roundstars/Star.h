//---------------------------------------------------------------------------
#ifndef StarH
#define StarH

#include "Image.h"
#include "Kernel.h"
#include "FullShape.h"
#include "RSParams.h"
#include <string>

//---------------------------------------------------------------------------

class Star : public Image {

public:

  Star(const Image& wholeimage, double _x, double _y, double pixscale,
    double _ixx, double _ixy, double _iyy, double _sky) : 
    Image(wholeimage,
      std::max(int(floor(_x-params.starradius/pixscale)),wholeimage.GetXMin()),
      std::min(int(ceil(_x+params.starradius/pixscale)),wholeimage.GetXMax()),
      std::max(int(floor(_y-params.starradius/pixscale)),wholeimage.GetXMin()),
      std::min(int(ceil(_y+params.starradius/pixscale)),wholeimage.GetYMax())),
    x(_x),y(_y),ixx(_ixx),ixy(_ixy),iyy(_iyy),sky(_sky) {}

  Star(const Star& rhs) : Image(rhs), x(rhs.x), y(rhs.y),
    ixx(rhs.ixx), ixy(rhs.ixy), iyy(rhs.iyy), sky(rhs.sky) {}
  ~Star() {}

  bool FindKernel(Kernel* kernel);
  bool MeasureShape(FullShape& shape,bool donewt,bool forisround=false);
  void MaskAperture();

  double GetX() const {return x;}
  double GetY() const {return y;}
  double GetIxx() const {return ixx;}
  double GetIxy() const {return ixy;}
  double GetIyy() const {return iyy;}
  double GetSky() const {return sky;}
  void SetX(double _x) {x = _x;}
  void SetY(double _y) {y = _y;}
  void SetIxx(double _ixx) {ixx = _ixx;}
  void SetIxy(double _ixy) {ixy = _ixy;}
  void SetIyy(double _iyy) {iyy = _iyy;}
  void SetSky(double _sky) {sky = _sky;}

private:

  double x,y;
  double ixx,ixy,iyy,sky;
};


#endif
