#ifdef HAVE_CMATH
#include <cmath>
#else
#include <math.h>
#endif

#ifdef USE_NAMESPACE
using namespace std;
#endif

int
main()
{
  isnan(1.0);
  return 0;
}

