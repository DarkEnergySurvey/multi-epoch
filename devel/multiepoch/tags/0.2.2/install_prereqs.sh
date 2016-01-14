# grep setup ups/multiepoch.table | awk -F"setupRequired" '{print $2}' | sed 's/[)(]//g' | awk '{printf "eups distrib install %s --nolocks\n",$0}' > install_prereqs.sh
eups distrib install mojo       0.2.5+0 --nolocks
eups distrib install despydb    2.0.1+0 --nolocks
eups distrib install despyastro 0.3.5+0  --nolocks
eups distrib install despymisc  1.0.1+0  --nolocks
eups distrib install despyfitsutils 1.0.1+0 --nolocks
eups distrib install pixcorrect 0.1.7+4 --nolocks
eups distrib install matplotlib 1.3.1+4 --nolocks
eups distrib install pandas 0.15.2+1 --nolocks
eups distrib install sextractor 2.19.5+1 --nolocks
eups distrib install stiff 2.1.3+1 --nolocks
eups distrib install swarp 2.38.0.1+0 --nolocks
eups distrib install psfex 3.17.3+1 --nolocks
eups distrib install scamp 2.2.5+2 --nolocks
