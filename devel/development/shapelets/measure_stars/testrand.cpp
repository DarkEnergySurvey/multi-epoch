#include <boost/random.hpp>
#include <ctime>
#include <iostream>
using namespace boost;

void SampleNormal (double mean, double sigma)
{
  // select random number generator
  mt19937 rng;
  // seed generator with #seconds since 1970
  rng.seed(static_cast<unsigned> (std::time(0)));

  // select desired probability distribution
  normal_distribution<double> norm_dist(mean, sigma);

  // bind random number generator to distribution, forming a function
  variate_generator<mt19937&, normal_distribution<double> >  normal_sampler(rng, norm_dist);

  for (int i=0; i<10; i++)
    std::cout<<"Random # = " << normal_sampler() << std::endl;

  return;
}

int main(int argc, char *argv)
{
  SampleNormal(0.0, 1.0);
}
