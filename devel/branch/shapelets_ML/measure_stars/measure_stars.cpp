#include "MeasureStars.hpp"
#include <stdlib.h>
#include <unistd.h>
#include <map>

//std::ostream* dbgout = &std::cout;
//bool XDEBUG = true;
std::ostream* dbgout = 0;
bool XDEBUG = false;

RSParameters params;
std::ostream* listout = 0;

extern char *optarg;
extern int optind, opterr;

bool FileTest(string file)
{
  std::fstream ftest;
  ftest.open(file.c_str(), std::ifstream::in);
  ftest.close();
  if (ftest.fail())
    return(false);
  else
    return(true);

}

std::map<std::string,ConvertibleString> ProcessArgs(int argc, char *argv[])
{

  // Output in our very flexible ConvertibleString map
  std::map<std::string,ConvertibleString> conf;

  conf["msconf"] = "";
  conf["fsconf"] = "";


  // Deal with options
  int c;
  static char optstring[] = "m:f:";

  while ( (c=getopt(argc, argv, optstring)) != -1 )
  {
    xdbg<<"c = "<<c<<std::endl;
    switch(c)
    {
        case 'm': 
            conf["msconf"] = optarg;
            break;
        case 'f':
            conf["fsconf"] = optarg;
            break;
        case '?':
            cout<<"Unknown option found, skipping"<<endl;
            break;

    }
  }
  xdbg<<"done with opts\n";

  // There must be arguments left
  int argleft = argc-optind;
  if (argleft <= 0)
  {
    cout<<"-usage: measure_stars -m msconf -f fsconf image_file cat_file allout starout"<<endl<<endl;
    cout<<"See the doc for measure_stars.py for more detailed info on usage"
        <<endl;
    exit(SYNTAXERROR);
  }
  else
  {
    conf["image_file"] = argv[optind];
    conf["cat_file"] = argv[optind+1];
    conf["allout"] = argv[optind+2];
    conf["starout"] = argv[optind+3];
  }
  xdbg<<"done assigning file names\n";

  // The msconf and fsconf may not have been entered
  if (conf["msconf"] == "" || conf["fsconf"] == "")
  {

    string config_dir;
    config_dir = getenv("DES_CONFIG_DIR");
    if (config_dir == "")
    {
      cout<<"Environment variable $DES_CONFIG_DIR not defined"<<endl;
      cout<<"Must be set or config files must be entered with -m and -f switches"<<endl;
      exit(SYNTAXERROR);
    }
  
    string def_msconf = config_dir + "/MeasureStars.conf";
    string def_fsconf = config_dir + "/FindStars.conf";

    if (conf["msconf"] == "")
      conf["msconf"] = def_msconf;
    if (conf["fsconf"] == "")
      conf["fsconf"] = def_fsconf;

  }
  xdbg<<"set msconf and fsconf\n";

  // Make sure all the files exist
  std::map<std::string,ConvertibleString>::iterator iconf;
  for (iconf=conf.begin(); iconf!=conf.end(); ++iconf)
  {  
    std::string key = iconf->first;
    std::string val = iconf->second;
    if (key != "allout" && key != "starout")
    {
      if (!FileTest(val))
      {
        cout<<
            "File: "<<key<<" = "<<val<<" not found. "<<
            "Exiting"<<endl;
        exit(READERROR);
      }
    }
  }
  xdbg<<"checked exist\n";

  return(conf);
}

int
main(int argc, char *argv[])
{
  dbg<<"Start measure_stars"<<std::endl;
 
  dbg<<"before processargs\n";
  std::map<std::string,ConvertibleString> conf=ProcessArgs(argc, argv);
  dbg<<"after processargs\n";

  MeasureStars ms(
          conf["msconf"], 
          conf["fsconf"],
          conf["image_file"],
          conf["cat_file"],
          conf["allout"],
          conf["starout"]);
  return(0);
}


 
