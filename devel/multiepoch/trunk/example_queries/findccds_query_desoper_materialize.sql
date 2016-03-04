with me as 
    (SELECT /*+ materialize */ 
         me.*,
         (case when me.CROSSRA0='Y' THEN abs(me.RACMAX - (me.RACMIN-360)) ELSE abs(me.RACMAX - me.RACMIN) END) as RA_SIZE,
         abs(me.DECCMAX - me.DECCMIN) as DEC_SIZE
         FROM felipe.me_images_Y2A1_FINALCUT_TEST me)
    SELECT
         me.FILENAME,
	 me.RA_CENT,me.DEC_CENT,
         me.CROSSRA0,
	 me.RA_SIZE,me.DEC_SIZE
    FROM
	 me,
         felipe.coaddtile_new tile
     WHERE
         tile.tilename = 'DES0311-5040' AND         
         (ABS(me.RA_CENT  -  tile.RA_CENT)  < (0.5*tile.RA_SIZE  + 0.5*me.RA_SIZE)) AND
         (ABS(me.DEC_CENT -  tile.DEC_CENT) < (0.5*tile.DEC_SIZE + 0.5*me.DEC_SIZE))
     order by me.RA_CENT;
