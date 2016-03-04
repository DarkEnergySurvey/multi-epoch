    SELECT
         tile.RA_SIZE, tile.DEC_SIZE,
         (case when me.CROSSRA0='Y' THEN abs(me.RACMAX - (me.RACMIN-360)) ELSE abs(me.RACMAX - me.RACMIN) END) as RA_SIZE,
         abs(me.DECCMAX - me.DECCMIN) as DEC_SIZE,
         me.FILENAME,
	 me.RA_CENT,me.DEC_CENT,
         me.CROSSRA0,
        ABS(me.RAC2  - me.RAC3 )  as RA_SIZE_CCD,
        ABS(me.DECC1 - me.DECC2 ) as DEC_SIZE_CCD	
     FROM
--         (select 
--            abs(me.DECCMAX - me.DECCMIN) as DEC_SIZE_CCD,
--            (case when me.CROSSRA0='Y' THEN abs(me.RACMAX - (me.RACMIN-360)) ELSE abs(me.RACMAX - me.RACMIN) END) as RA_SIZE_CCD
--	  from 
--	    felipe.me_images_Y2A1_FINALCUT_TEST me) im, 
         felipe.coaddtile_new tile,
         felipe.me_images_Y2A1_FINALCUT_TEST me
     WHERE
         tile.tilename = 'DES0311-5040' AND         
	 me.CROSSRA0='N' AND
--         (ABS(me.RA_CENT  -  tile.RA_CENT)  < (0.5*tile.RA_SIZE  + 0.5*im.RA_SIZE_CCD)) AND
--         (ABS(me.DEC_CENT -  tile.DEC_CENT) < (0.5*tile.DEC_SIZE + 0.5*im.DEC_SIZE_CCD))
         (ABS(me.RA_CENT  -  tile.RA_CENT)  < (0.5*tile.RA_SIZE  + 0.5*ABS(me.RAC2 - me.RAC3) )) AND
         (ABS(me.DEC_CENT -  tile.DEC_CENT) < (0.5*tile.DEC_SIZE + 0.5*ABS(me.DECC1- me.DECC2)))
     order by me.RA_CENT;

--         (ABS(me.RA_CENT  -  47.912207498)  < (0.5*1.1615532891  + 0.5*ABS(me.RAC2 - me.RAC3) )) AND
--         (ABS(me.DEC_CENT -  -50.6694444444) < (0.5*0.7304356589 + 0.5*ABS(me.DECC1- me.DECC2)));
