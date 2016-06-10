with me as 
    (SELECT /*+ materialize */ 
         me.*,
         (case when me.CROSSRA0='Y' THEN abs(me.RACMAX - (me.RACMIN-360)) ELSE abs(me.RACMAX - me.RACMIN) END) as RA_SIZE_CCD,
         abs(me.DECCMAX - me.DECCMIN) as DEC_SIZE_CCD
         FROM felipe.me_images_Y2T9_FINALCUT_TEST me)
    SELECT
         me.FILENAME,
	 me.RA_CENT,me.DEC_CENT,
         me.CROSSRA0,
	 me.RA_SIZE_CCD,me.DEC_SIZE_CCD
    FROM
	 me,
         felipe.coaddtile_new tile
     WHERE
         tile.tilename = 'DES0516-5457' AND         
         (ABS(me.RA_CENT  -  tile.RA_CENT)  < (0.5*tile.RA_SIZE  + 0.5*me.RA_SIZE_CCD)) AND
         (ABS(me.DEC_CENT -  tile.DEC_CENT) < (0.5*tile.DEC_SIZE + 0.5*me.DEC_SIZE_CCD))
     order by me.RA_CENT;
