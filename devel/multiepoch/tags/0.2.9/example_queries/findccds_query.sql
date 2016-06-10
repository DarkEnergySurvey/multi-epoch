     SELECT
         me.FILENAME,me.COMPRESSION,me.PATH,me.BAND,me.UNITNAME,
         me.RACMIN,me.RACMAX,
         me.DECCMIN,me.DECCMAX,
         me.RA_SIZE,me.DEC_SIZE,
         me.RA_CENT, me.RAC1,  me.RAC2,  me.RAC3,  me.RAC4,
         me.DEC_CENT,me.DECC1, me.DECC2, me.DECC3, me.DECC4
     FROM
         felipe.me_images_Y2A1_FINALCUT_TEST me,
	 felipe.coaddtile_new tile		     
     WHERE
	 tile.tilename = 'DES0311-5040' AND
         (ABS(me.RA_CENT  -  tile.RA_CENT)  < (0.5*tile.RA_SIZE  + 0.5*me.RA_SIZE)) AND
         (ABS(me.DEC_CENT -  tile.DEC_CENT) < (0.5*tile.DEC_SIZE + 0.5*me.DEC_SIZE));
