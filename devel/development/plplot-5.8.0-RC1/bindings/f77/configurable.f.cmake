C      $Id: configurable.f.cmake 7916 2007-10-01 02:52:18Z airwin $
C
C      Copyright (C) 2004  Alan W. Irwin
C
C      This file is part of PLplot.
C
C      PLplot is free software; you can redistribute it and/or modify
C      it under the terms of the GNU General Library Public License as
C      published by the Free Software Foundation; either version 2 of the
C      License, or (at your option) any later version.
C
C      PLplot is distributed in the hope that it will be useful,
C      but WITHOUT ANY WARRANTY; without even the implied warranty of
C      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
C      GNU Library General Public License for more details.
C
C      You should have received a copy of the GNU Library General Public
C      License along with PLplot; if not, write to the Free Software
C      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

      subroutine plparseopts(mode)
      implicit none
      include 'sfstubs.h'
      integer mode
      integer maxargs, iargs, numargs, index, maxindex, iargc, islen
      parameter(maxindex = maxlen/4)
      parameter (maxargs=20)
      character*(maxlen) arg
      integer*4 iargsarr(maxindex, maxargs)
@HAVE_F77PARSE_CL_FALSE@      write(0,'(a)') 'plparseopts not implemented on this fortran'//
@HAVE_F77PARSE_CL_FALSE@     & ' platform because iargc or getarg are not available'
@HAVE_F77PARSE_CL_TRUE@      numargs = iargc()
@HAVE_F77PARSE_CL_TRUE@      if(numargs.lt.0) then
@HAVE_F77PARSE_CL_TRUE@C       This actually happened on badly linked Cygwin platform.
@HAVE_F77PARSE_CL_TRUE@        write(0,'(a)') 'plparseopts: negative number of arguments'
@HAVE_F77PARSE_CL_TRUE@        return
@HAVE_F77PARSE_CL_TRUE@      endif
@HAVE_F77PARSE_CL_TRUE@      if(numargs+1.gt.maxargs) then
@HAVE_F77PARSE_CL_TRUE@        write(0,'(a)') 'plparseopts: too many arguments'
@HAVE_F77PARSE_CL_TRUE@        return
@HAVE_F77PARSE_CL_TRUE@      endif
@HAVE_F77PARSE_CL_TRUE@      do 10 iargs = 0, numargs
@HAVE_F77PARSE_CL_TRUE@        call getarg(iargs, arg)
@HAVE_F77PARSE_CL_TRUE@        call plstrf2c(arg(:islen(arg)), string1, maxlen)
@HAVE_F77PARSE_CL_TRUE@        do 5 index = 1, maxindex
@HAVE_F77PARSE_CL_TRUE@          iargsarr(index, iargs+1) = s1(index)
@HAVE_F77PARSE_CL_TRUE@    5     continue
@HAVE_F77PARSE_CL_TRUE@   10   continue
@HAVE_F77PARSE_CL_TRUE@      call plparseopts7(numargs+1, iargsarr, mode, maxindex*4)
      end
