// tester.cpp
// Program to test ConfigFile class

#include <string>
#include <iostream>
#include "ConfigFile.h"
#include "Triplet.h"

using std::string;
using std::cout;
using std::endl;

string title = "";
bool success = true;
int errors = 0;

void announce( const string& name )
{
  title = name;
  success = true;
  cout << "Test " << title << ":" << endl;
}

void judge()
{
  if( success )
  {
    cout << "Passed " << title << "." << endl << endl;
    return;
  }
  cout << "Error - Failed " << title << "." << endl << endl;
  ++errors;
}

void evaluate( const bool& test )
{
  success = success && test;
}

int main( void )
{
  cout << "This is a test of the ConfigFile class." << endl;

  try {

    ConfigFile cf( "test.inp" );

    std::string extra_config = "extra_val = 100";
    std::stringstream ss(extra_config);
    ss >> cf;

    cout << "Here is the configuration read from test.inp:" << endl;
    cout << cf << endl;

    // Test reading of basic types

    announce("reading of basic types");

    int valInt = cf["integer"];
    cout << "Value of 'integer' is " << valInt << endl;
    evaluate( valInt == 7 );

    double valDbl = cf["double"];
    cout << "Value of 'double' is " << valDbl << endl;
    evaluate( valDbl == 1.99 );

    bool valBoo = cf["boolean"];
    cout << "Value of 'boolean' is " << valBoo << endl;
    evaluate( valBoo );

    string valStr = cf["string"];
    cout << "Value of 'string' is " << valStr << endl;
    evaluate( valStr == "one fine day" );

    int xval1 = cf["extra_val"];
    cout << "Value from extra configuration string #1 is "<<xval1<<endl;
    evaluate( xval1 == 100 );

    extra_config = "extra_val = 200";
    cf.Append(extra_config);

    int xval2 = cf["extra_val"];
    cout << "Value from extra configuration string #2 is "<<xval2<<endl;
    evaluate( xval2 == 200 );

    judge();

    // Test reading by different methods

    announce("reading by different methods");

    int methodNormal = cf["integer"];
    cout << "Read integer normal way as " << methodNormal << endl;
    evaluate( methodNormal == 7 );

    int methodExplicit = cf.read<int>( "integer" );
    cout << "Read integer explicitly as " << methodExplicit << endl;
    evaluate( methodExplicit == 7 );

    int methodDefault = cf.read( "integer", 0 );
    cout << "Read integer with default as " << methodDefault << endl;
    evaluate( methodDefault == 7 );

    int valInto = 0;
    bool methodInto = cf.readInto( valInto, "integer" );
    cout << "Read integer into variable as " << valInto << endl;
    evaluate( methodInto );

    methodInto = cf.readInto( valInto, "integer", 0 );
    cout << "Read integer into variable with default as " << valInto << endl;
    evaluate( methodInto );

    judge();

    // Test interpretation as different types

    announce("interpretation as different types");

    string typeStr = cf["weight"];
    cout << "Value of weight as a string is " << typeStr << endl;
    evaluate( typeStr == "2.5 kg" );

    double typeDbl = cf["weight"];
    cout << "Value of weight as a double is " << typeDbl << endl;
    evaluate( typeDbl == 2.5 );

    int typeInt = cf["weight"];
    cout << "Value of weight as an integer is " << typeInt << endl;
    evaluate( typeInt == 2 );

    judge();

    // Test reading of user-defined types

    announce("reading of user-defined types");

    Triplet trip = cf["triplets"];
    cout << "First Triplet in 'triplets' is " << trip << endl;
    evaluate( trip.a==1 && trip.b==2 && trip.c==3 );
    std::vector<Triplet> trips = cf["triplets"];
    //cout << "First Triplet in 'triplets' is " << trips[0] << endl;
    cout << "Second Triplet in 'triplets' is " << trips[1] << endl;
    cout << "Third Triplet in 'triplets' is " << trips[2] << endl;
    evaluate( trips[0].a==1 && trips[0].b==2 && trips[0].c==3 );
    evaluate( trips[1].a==4 && trips[1].b==5 && trips[1].c==6 );
    evaluate( trips[2].a==7 && trips[2].b==8 && trips[2].c==9 );

    judge();

    // Test reading of repeated keys

    announce("reading of repeated keys");

    int repeat = cf.read<int>( "repeated" );
    cout << "Value of 'repeated' is " << repeat << endl;
    evaluate( repeat == 2 );

    judge();

    // Test case-sensitivity of key recognition

    announce("case-sensitivity of key recognition");

    int oneStall = cf.read<int>( "oneStall" );
    cout << "Value of oneStall is " << oneStall << endl;
    evaluate( oneStall == 1 );

    int onesTall = cf.read<int>( "onesTall" );
    cout << "Value of onesTall is " << onesTall << endl;
    evaluate( onesTall == 111 );

    judge();

    // Test recognition of keys with embedded spaces

    announce("recognition of keys with embedded spaces");

    bool spaceInKey = cf.read<bool>( "space key", false );
    cout << "Value of 'space key' is " << spaceInKey << endl;
    evaluate( spaceInKey );

    judge();

    // Test legality of all-space values

    announce("legality of all-space values");

    string noValue = cf.read<string>( "noValue", "something" );
    cout << "Value of 'noValue' is " << noValue << endl;
    evaluate( noValue == "" );

    judge();

    // Test legality of all-space keys

    announce("legality of all-space keys");

    int spaceKey = cf.read<int>( "" );
    cout << "Value of nothing is " << spaceKey << endl;
    evaluate( spaceKey == 5 );

    judge();

    // Test reading of values that include a delimiter

    announce("reading of values that include a delimiter");

    string equation = cf.read<string>( "equation" );
    cout << "Value of 'equation' is " << equation << endl;
    evaluate( equation == "y = mx + b" );

    judge();

    // Test arrays
    announce("reading zero-length arrays");

    std::vector<int> v0a = cf["v0a"];
    std::vector<int> v0b = cf["v0b"];
    std::vector<int> v0c = cf["v0c"];
    evaluate(v0a.size() == 0);
    evaluate(v0b.size() == 0);
    evaluate(v0c.size() == 0);

    judge();

    announce("reading one-length arrays");

    std::vector<int> v1a = cf["v1a"];
    std::vector<int> v1b = cf["v1b"];
    std::vector<int> v1c = cf["v1c"];
    evaluate(v1a.size() == 1 && v1a[0] == 1);
    evaluate(v1b.size() == 1 && v1b[0] == 1);
    evaluate(v1c.size() == 1 && v1c[0] == 1);

    judge();

    announce("reading longer arrays");

    std::vector<int> v2 = cf["v2"];
    std::vector<int> v3 = cf["v3"];
    std::vector<int> v6 = cf["v6"];
    std::vector<int> v9 = cf["triplets"];
    evaluate(v2.size() == 2 && v2[1] == 2);
    evaluate(v3.size() == 3 && v3[2] == 3);
    evaluate(v6.size() == 6 && v6[5] == 6);
    evaluate(v9.size() == 9 && v9[8] == 9);

    judge();

    // Test termination of multiple-line values by blank lines

    announce("termination of multiple-line values by blank lines");

    string multiPause = cf.read<string>( "multilinePause" );
    cout << "Value of 'multilinePause' is " << multiPause << endl;
    evaluate( multiPause.find("third")  != string::npos &&
	multiPause.find("fourth") == string::npos );

    judge();

    // Test continuation of multiple-line values after comments

    announce("continuation of multiple-line values after comments");

    string multiComment = cf.read<string>( "multilineComment" );
    cout << "Value of 'multilineComment' is " << multiComment << endl;
    evaluate( multiComment.find("fourth") != string::npos );

    judge();

    // Test skipping of commented lines in multiple-line values

    announce("skipping of commented lines in multiple-line values");

    string multiSkip = cf.read<string>( "multilineSkip" );
    cout << "Value of 'multilineSkip' is " << multiSkip << endl;
    evaluate( multiSkip.find("third")  == string::npos &&
	multiSkip.find("fourth") != string::npos );

    judge();

    // Test skipping of assignments within comments

    announce("skipping of assignments within comments");

    int postComment = cf.read<int>( "postComment", 0 );
    cout << "Value of 'postComment' is " << postComment << endl;
    evaluate( postComment == 0 );

    judge();

    // Test alternative delimiters

    announce("alternative delimiters");
    string cfDelim = cf.getDelimiter();

    int atDelimiter = cf.read<int>( "atDelimiter", 0 );
    cout << "Value of 'atDelimiter' with '" << cfDelim;
    cout << "' delimiter is " << atDelimiter << endl; 
    evaluate( atDelimiter == 0 );

    ConfigFile atcf( "test.inp", "@" );
    atDelimiter = atcf.read<int>( "atDelimiter", 0 );
    cout << "Value of 'atDelimiter' with '" << atcf.getDelimiter();
    cout << "' delimiter is " << atDelimiter << endl; 
    evaluate( atDelimiter == 7 );

    judge();

    // Test alternative comment separators

    announce("alternative comment separators");
    string cfComm = cf.getComment();

    int altComment = cf.read<int>( "! alternateComment", 0 );
    cout << "Value of '! alternateComment' with '" << cfComm;
    cout << "' comment separator is " << altComment << endl; 
    evaluate( altComment == 9 );

    altComment = cf.read<int>( "alternateComment", 0 );
    cout << "Value of 'alternateComment'   with '" << cfComm;
    cout << "' comment separator is " << altComment << endl; 
    evaluate( altComment == 0 );

    ConfigFile excf( "test.inp", cf.getDelimiter(), "!" );
    altComment = excf.read<int>( "! alternateComment", 0 );
    cout << "Value of '! alternateComment' with '" << excf.getComment();
    cout << "' comment separator is " << altComment << endl; 
    evaluate( altComment == 0 );

    altComment = excf.read<int>( "alternateComment", 0 );
    cout << "Value of 'alternateComment'   with '" << excf.getComment();
    cout << "' comment separator is " << altComment << endl; 
    evaluate( altComment == 0 );

    judge();

    // Test legality of a space as a delimiter

    announce("legality of a space as a delimiter");

    int spaceDelimiter = cf.read<int>( "spaceDelimiter", 0 );
    cout << "Value of 'spaceDelimiter' with '" << cfDelim;
    cout << "' delimiter is " << spaceDelimiter << endl; 
    evaluate( spaceDelimiter == 0 );

    ConfigFile spcf( "test.inp", " " );
    spaceDelimiter = spcf.read<int>( "spaceDelimiter", 0 );
    cout << "Value of 'spaceDelimiter' with '" << spcf.getDelimiter();
    cout << "' delimiter is " << spaceDelimiter << endl; 
    evaluate( spaceDelimiter == 7 );

    judge();

    // Test interaction of assignments with end of file sentry

    announce("interaction of assignments with end of file sentry");

    string endStr = cf.read<string>( "end" );
    cout << "Value of 'end' with '" << cf.getSentry();
    cout << "' sentry is " << endStr << endl;
    evaluate( endStr == "before uncommented sentry" );

    ConfigFile eofcf( "test.inp", cfDelim, cfComm, "" );
    endStr = eofcf.read<string>( "end" );
    cout << "Value of 'end' with '" << eofcf.getSentry();
    cout << "' sentry is " << endStr << endl;
    evaluate( endStr == "before EOF" );
    judge();

    // Report results

  }
  catch( ConfigFile::file_not_found& e ) {
    cout << "Error - File '" << e.filename << "' not found.";
    cout << endl << endl;
    ++errors;
  }
  catch( ConfigFile::key_not_found& e ) {
    cout << "Error - Key '" << e.key << "' not found.";
    cout << endl << endl;
    ++errors;
  } 
  catch( std::string& e ) {
    cout << "Error - "<<e<<endl;
    ++errors;
  }


  if( errors > 0 )
  {
    cout << "Failed " << errors << " tests of ConfigFile.\n";
    cout << "Please send a copy of this output to wagnerr@umich.edu.\n";
    return 1;
  }

  cout << "Passed all tests of ConfigFile." << endl;

  return 0;
}
