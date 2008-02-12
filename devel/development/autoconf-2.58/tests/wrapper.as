# wrapper.as -- running `$0' as if it were installed.   -*- shell-script -*-
# @configure_input@
# Copyright (C) 2003 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

AS_INIT[]dnl

testdir=@abs_top_builddir@/tests
AUTOCONF=$testdir/autoconf
AUTOHEADER=$testdir/autoheader
AUTOM4TE=$testdir/autom4te
AUTOM4TE_CFG=@abs_top_builddir@/lib/autom4te.cfg
autom4te_perllibdir=@abs_top_srcdir@/lib
export AUTOCONF AUTOHEADER AUTOM4TE AUTOM4TE_CFG autom4te_perllibdir

case $as_me in
  ifnames)
     # Does not have lib files.
     exec @abs_top_builddir@/bin/$as_me ${1+"$@"}
     ;;
  *)
     # We might need files from the build tree (frozen files), in
     # addition of src files.
     exec @abs_top_builddir@/bin/$as_me \
	  -B @abs_top_builddir@/lib \
	  -B @abs_top_srcdir@/lib ${1+"$@"}
esac
exit 1
