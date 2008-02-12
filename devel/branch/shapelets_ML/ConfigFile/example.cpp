// example.cpp
// Program to demonstrate ConfigFile class

#include <string>
#include <iostream>
#include "ConfigFile.h"
#include "Triplet.h"

using std::string;
using std::cout;
using std::endl;

int main( void )
{
  // A configuration file can be loaded with a simple

  ConfigFile config( "example.inp" );

  // Values can be read from the file by name

  int apples = config["apples"];
  cout << "The number of apples is " << apples << endl;

  double price = config["price"];
  cout << "The price is $" << price << endl;

  string title = config["title"];
  cout << "The title of the song is " << title << endl;

  // We can provide default values in case the name is not found

  int oranges = config.read("oranges", 0 );
  cout << "The number of oranges is " << oranges << endl;

  int fruit = 0;
  fruit += config.read( "apples", 0 );
  fruit += config.read( "pears", 0 );
  fruit += config.read( "oranges", 0 );
  cout << "The total number of apples, pears, and oranges is ";
  cout << fruit << endl;

  // For backwards compatibility, we keep the old template version:

  int pears = config.read<int>( "pears" );
  cout << "The number of pears is " << pears;
  cout << ", but you knew that already" << endl;

  // The value is interpreted as the requested data type

  cout << "The weight is ";
  cout << string(config["weight"]);
  cout << " as a string" << endl;

  cout << "The weight is ";
  cout << double(config["weight"]);
  cout << " as a double" << endl;

  cout << "The weight is ";
  cout << int(config["weight"]);
  cout << " as an integer" << endl;

  // When reading boolean values, a wide variety of words are
  // recognized, including "true", "yes", and "1"

  if( config["sale"] )
    cout << "The fruit is on sale" << endl;
  else
    cout << "The fruit is full price" << endl;

  // A std::vector array of values can be marked using {a,b,c} notation
  std::vector<double> box_size = config["box size"];
  cout << "The dimensions of the box are: ";
  cout << box_size[0] << " x " << box_size[1] << " x " << box_size[2] << endl;

  // We can also read user-defined types, as long as the input and
  // output operators, >> and <<, are defined

  Triplet point = config["zone"];
  cout << "The first point in the zone is " << point << endl;

  std::vector<Triplet> points = config["zone"];
  cout << "The complete zone is: \n";
  for(size_t i=0;i<points.size();i++) cout<<points[i]<<std::endl;

  // The readInto() functions report whether the named value was found

  int pommes = 0;
  if( config.readInto( pommes, "pommes" ) )
    cout << "The input file is in French:  ";
  else if( config.readInto( pommes, "apples" ) )
    cout << "The input file is in English:  ";
  cout << "The number of pommes (apples) is " << pommes << endl;

  // Named values can be added to a ConfigFile

  config["squash"] = 13;
  int squash = config["squash"];
  cout << "The number of squash was set to " << squash << endl;

  // Also, the old version for backwards compatibility:
  
  config.add( "zucchini", 12 );
  int zucchini = config.read( "zucchini", 0 );
  cout << "The number of zucchini was set to " << zucchini << endl;

  // And values can be removed

  config.remove( "pears" );
  if( config.readInto( pears, "pears" ) )
    cout << "The pears are ready" << endl;
  else
    cout << "The pears have been eaten" << endl;

  // An entire ConfigFile can written (and restored)

  cout << "Here is the modified configuration file:" << endl;
  cout << config;

  return 0;
}
