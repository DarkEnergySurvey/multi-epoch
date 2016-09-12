grep setup ups/multiepoch.table | awk -F"setupRequired" '{print $2}' | sed 's/[)(]//g' | awk '{printf "eups distrib install %s --nolocks\n",$0}'
