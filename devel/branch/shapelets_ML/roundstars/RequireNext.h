//---------------------------------------------------------------------------
#ifndef RequireNextH
#define RequireNextH
//---------------------------------------------------------------------------

#define require_next(x) 			\
	  if (i==argc-1) x;			\
	  else
// Note: require_next just requires that there is another argument following
//   the flag argument.  Think of the syntaxt as follows:
// require_next(what to do if it's not there) {what to do if it is there}

#endif
