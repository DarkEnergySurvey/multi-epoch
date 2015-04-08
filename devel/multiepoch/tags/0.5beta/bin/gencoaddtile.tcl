proc whereplane {try} {


loop dec 30 -90 -10 {
set minl 999
set minb 999 
set minra 0
set mindec 0

loop ra 0 360 10 {

	set lb [eqToGal $ra $dec]
	set l [keylget lb gLong]
	set b [keylget lb gLat]
	if {$l > 180} {set l [expr $l-180]}
	if {abs($b) < $minb} {
			set minb [expr abs($b)]
			set minra $ra
			set mindec $dec
	}
	
}

echo $minra $mindec  [eqToGal $minra $mindec]  [eqToGal $try $mindec]

}


}

#coaddtile_id,project,tilename,ra,dec,equinox,pixelsize,npix_ra,npix_dec
#13984,DES,DES2339-5149,354.8578491,-51.8333321,2000,.27,10000,10000

#include unique ra/dec limits
#plot results
#start ra at the plane/gc
#start dec at 0 and go up
#start dec at 0 and go down
#get rid of RA=360 wrap around error

proc gencoaddtile {out} {

	set fd [open $out w]
    puts $fd "coaddtile_id,project,tilename,ra,dec,equinox,pixelsize,npix_ra,npix_dec,rall,decll,raul,decul,raur,decur,ralr,declr,urall,udecll,uraur,udecur"
#13984,DES,DES2339-5149,354.8578491,-51.8333321,2000,.27,10000,10000

   set dec0 30
   set dec1 -89.99
   set ra0 275
   set ra1 [expr $ra0+360]
   set id 20000

	# one arcminute overlap, works everywhere but close to pole (dec < -75 deg)
   set overlap1 [expr 1.0/60.] 
   set overlap2 [expr 2.0/60.] 
   set overlap5 [expr 5.0/60.] 
   set overlap10 [expr 10.0/60.] 

   set scale 0.263
   set pixwidth 10000
   set done 0
   set dec $dec0

   while {!$done} {
     if {$dec > -74} {
	set overlap $overlap1
     } elseif {$dec > -82.5} {
	set overlap $overlap2
     } elseif {$dec > -87.6} {
	set overlap $overlap5
    } else {
	set overlap $overlap10
    }
   
   	set degwidth [expr $scale*$pixwidth/3600.0]
   	set incrdec [expr -1.0*($degwidth-$overlap)]
   	set dechalfwidth [expr $degwidth/2.0]
   	set udechalfwidth [expr 0.5*($degwidth-$overlap)]

     set ra $ra0
     set firstra 1

     while {$ra < $ra1} {
      set decu [expr $dec-$dechalfwidth]
      set rauincr [expr 0.5*$degwidth/cos(3.1415926*$decu/180.0)]
      set raul [expr $ra-$rauincr]
      set raur [expr $ra+$rauincr]

      set decl [expr $dec+$dechalfwidth]

      set udecl [expr $dec-$udechalfwidth]
      set udecu [expr $dec+$udechalfwidth]

      set ralincr [expr 0.5*$degwidth/cos(3.1415926*$decl/180.0)]
      set rall [expr $ra-$ralincr]
      set ralr [expr $ra+$ralincr]

      set raaincr [expr 0.5*($degwidth-$overlap)/cos(3.1415926*$dec/180.0)]
      set ural [expr $ra-$raaincr]
      set urau [expr $ra+$raaincr]

      if {$raul > 360 && $raur > 360 && $rall > 360 && $ralr > 360 && $ural > 360 && $urau > 360} {
		set raul [expr $raul-360.0]
		set raur [expr $raur-360.0]
		set rall [expr $rall-360.0]
		set ralr [expr $ralr-360.0]
		set ural [expr $ural-360.0]
		set urau [expr $urau-360.0]
	}
	if {$firstra == 1} {
		set saveural $ural
		set firstra 0
	}
		if {$ra > 360} {
			set raavgp [expr $ra-360.0]
		} else {
			set raavgp $ra
		}


      set rahh [csubstr [degToHMS $raavgp] 1 2]
      set ramm [csubstr [degToHMS $raavgp] 4 2]
    if {$dec > 0} {
      set decsign "+"
      set dedd [csubstr [degToDMS $dec] 1 2]
      set demm [csubstr [degToDMS $dec] 4 2]
    } else {
      set decsign "-"
      set dedd [csubstr [degToDMS $dec] 1 2]
      set demm [csubstr [degToDMS $dec] 4 2]
	}
	set name DES$rahh$ramm$decsign$dedd$demm

      set newra [expr $ra+($degwidth-$overlap)/cos(3.1415926*$dec/180.0)]

	if {$newra > $ra1} {
		set urau $saveural
	}

puts $fd "$id,DES,$name,[format %.6f $raavgp],[format %.6f $dec],2000,0.263,$pixwidth,$pixwidth,[format %.6f $raul],[format %.6f $decu],[format %.6f $rall],[format %.6f $decl],[format %.6f $ralr],[format %.6f $decl],[format %.6f $raur],[format %.6f $decu],[format %.6f $ural],[format %.6f $udecl],[format %.6f $urau],[format %.6f $udecu]"
	
	set ra $newra

#coaddtile_id,project,tilename,ra,dec,equinox,pixelsize,npix_ra,npix_dec
#13984,DES,DES2339-5149,354.8578491,-51.8333321,2000,.27,10000,10000
	
     incr id
	}
     set dec [expr $dec+$incrdec]
     if {$dec < $dec1} {
		echo dec $dec1 $dec
		set done 1
     }
   }

close $fd
}

