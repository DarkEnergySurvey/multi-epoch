To EUPS setuo

setup -v -r ~/DESDM-Code/devel/multiepoch/branches/me_mojo

Code descriptions:

- gencoaddtile.tcl:
  Original tck code from Brian Yanny

- gencoaddtile.py: 
  This is the *exact* translation of the TCL code gencoaddtile.tcl
  Creates file: coaddtiles_table_reproduced.dat

- gencoaddtile_plots.py:
  gencoaddtile_plots_all_SKY.py:

  Plot the tiles re-generated from gencoaddtile.py reading them from
  the file: coaddtiles_table_reproduced.dat

- gencoaddtile_newcorners.py:
  Creates the COADDTILE table with the new corners and new schema,

- compare_corners_SWarp2Database.py:
  Compare the corners from a SWarped TILE fitsfile, against the values
  stored in the database table

- insert_CCDcorners_toIMAGE.py:
  Script to insert CCD corners into the NEW DB schema. Not needed
  anymore, as they are written automatically by the Framework

- insert_CCDcorners_toIMAGE_oldSchema.py:
  Script to insert CCD corners into the old DB schema.	

Libraries/Classes

- descoords.py:
  Proto-type class to handle coords in new TEST DB and compute
  coordinates for tiles
 
- DESfits.py  
  A fitsio-based class to handle DES MEF files
