#ifndef _Errors_hpp
#define _Errors_hpp

// Exit codes
#define READERROR 35
#define FORMATERROR 36
#define SYNTAXERROR 37
#define RANGEERROR 38
#define CATALOGERROR 39
#define PARAMETERERROR 40
#define ALGORITHMERROR 41

void ErrorExit(const char *message, int error_code);

#endif
