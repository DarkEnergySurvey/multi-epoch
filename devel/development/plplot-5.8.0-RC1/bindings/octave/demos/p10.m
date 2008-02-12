## Copyright (C) 1998-2003 Joao Cardoso.
## 
## This program is free software; you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by the
## Free Software Foundation; either version 2 of the License, or (at your
## option) any later version.
## 
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## This file is part of plplot_octave.

    
function p10

  if (exist("automatic_replot"))
    t = automatic_replot;
    automatic_replot = 0;
  endif

  title("Comet");
  xlabel "";
  ylabel "";
  t = -pi:pi/200:0;
  comet(t,tan(sin(t))-sin(tan(t)));
  
  if (exist("automatic_replot"))
    automatic_replot = t;
  endif

endfunction

