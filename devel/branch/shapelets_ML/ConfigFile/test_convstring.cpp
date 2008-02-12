#include <map>
#include <string>
#include <iostream>
#include <iomanip>
#include "ConfigFile.h"

struct foo {
  double x;
  int y;
};

std::istream& operator>>(std::istream& is, foo& f)
{ 
  char c;
  is >> c; 
  if (c != '(') { is.setstate(std::ios::badbit); return is; }
  is >> f.x >> c;
  if (c != ',') { is.setstate(std::ios::badbit); return is; }
  is >> f.y >> c;
  if (c != ')') { is.setstate(std::ios::badbit); return is; }
  return is; 
}

std::ostream& operator<<(std::ostream& os, const foo& f)
{ os << "(" << f.x << "," << f.y << ")"; return os; }

int main(int argc, char *argv)
{
  
  std::map<std::string,ConvertibleString> sm;

  sm["string"] = "hello world";
  sm["double"] = "25.1";
  sm["int"] = "35";
  sm["bool"] = "true";
  sm["foo"] = "(1.7,23)";
  
  try
  {
    std::string s = sm["string"];
    double d = sm["double"];
    int i = sm["int"];
    bool b = sm["bool"];
    foo f = sm["foo"];

    std::cout<<std::boolalpha;
    std::cout<<"Constructors:\n";
    std::cout<<"sm[\"string\"] = "<<s<<std::endl;
    std::cout<<"sm[\"double\"] = "<<d<<std::endl;
    std::cout<<"sm[\"int\"] = "<<i<<std::endl;
    std::cout<<"sm[\"bool\"] = "<<b<<std::endl;
    std::cout<<"sm[\"foo\"] = "<<f<<std::endl;

    sm["string"] = "goodbye cruel world";
    sm["double"] = 18.9;
    sm["int"] = -20;
    sm["bool"] = false;
    foo bar;
    bar.x = 8.9; bar.y = 30;
    sm["foo"] = bar;

    d = sm["double"];
    i = sm["int"];
    b = sm["bool"];
    s = sm["string"];
    f = sm["foo"];

    std::cout<<"Assignment:\n";
    std::cout<<"sm[\"string\"] = "<<s<<std::endl;
    std::cout<<"sm[\"double\"] = "<<d<<std::endl;
    std::cout<<"sm[\"int\"] = "<<i<<std::endl;
    std::cout<<"sm[\"bool\"] = "<<b<<std::endl;
    std::cout<<"sm[\"foo\"] = "<<f<<std::endl;
  }
  catch (std::string s)
  {
    std::cout<<"Error: "<<s<<std::endl;
  }
}
