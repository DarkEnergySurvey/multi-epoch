#include "Errors.hpp"
#include <iostream>


void ErrorExit(const char *message, int error_code)
{
  std::cout<<
        "Error: "<<message<<
        " Exiting with error code: "<<error_code<<std::endl;
    exit(error_code);
}

