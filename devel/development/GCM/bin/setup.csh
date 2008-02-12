#! /bin/csh -f
# Note that this file should be "sourced" rather than executed.

if (! $?JAVA_HOME) then
    echo -n "Where is the top level directory of your Java SDK distribution?"
    set ans = "$<"
    if ($ans != "") setenv JAVA_HOME $ans
endif

if (! $?DESGCM_HOME) then
    echo -n "Where is the top level directory of your DESGCM distribution?"
    set ans = "$<"
    if ($ans != "") setenv DESGCM_HOME $ans
endif

set installation = ("DES GCM" "Java SDK")
set installdir = ($DESGCM_HOME $JAVA_HOME)
unset badconfig
foreach i (1 2)
   if (! -d $installdir[$i]) then
       set badconfig = 1
       echo "Can't find $installation[$i] directory: $installdir[$i]"
   endif
end
if ($?badconfig) then
   unsetenv DESGCM_HOME
   unsetenv JAVA_HOME
   exit(1)
endif
echo DESGCM_HOME:  $DESGCM_HOME
echo JAVA_HOME:  $JAVA_HOME

set path = ($JAVA_HOME/bin $DESGCM_HOME/bin $path)

set lib = $DESGCM_HOME/lib

setenv CLASSPATH .
setenv CLASSPATH ${CLASSPATH}:$lib
foreach jar ($lib/*.jar)
    setenv CLASSPATH ${CLASSPATH}:$jar
end

echo CLASSPATH:  $CLASSPATH
