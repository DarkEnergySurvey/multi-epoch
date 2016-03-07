with ima as 
    (SELECT /*+ materialize */ 
         FILENAME, FILETYPE, CROSSRA0,PFW_ATTEMPT_ID,
         BAND,CCDNUM,
	 DEC_CENT,	
	 (case when RA_CENT > 180. THEN RA_CENT-360. ELSE RA_CENT END) as RA_CENT,
         (case when CROSSRA0='Y' THEN abs(RACMAX - (RACMIN-360)) ELSE abs(RACMAX - RACMIN) END) as RA_SIZE_CCD,
         abs(DECCMAX - DECCMIN) as DEC_SIZE_CCD
       FROM image)
    SELECT
	 ima.FILENAME,
	 ima.RA_CENT,ima.DEC_CENT,
	 tile.RA_CENT-360,
	 tile.DEC_CENT,
         ima.BAND,
	 ima.RA_SIZE_CCD,ima.DEC_SIZE_CCD
    FROM
	 ima, ops_proctag, felipe.coaddtile_new tile
    WHERE
         ima.FILETYPE    = 'red_immask' AND
	 ima.PFW_ATTEMPT_ID = ops_proctag.PFW_ATTEMPT_ID AND	
--	 Change TAG accordlingly	 
         ops_proctag.TAG = 'Y2A1_FINALCUT' AND
--       Optionally exclude blacklist
--	 ima.filename NOT IN (select filename from image i, GRUENDL.MY_BLACKLIST b where i.expnum=b.expnum and i.ccdnum=b.ccdnum) AND
--	 Change tilename accordingly (examples)
	 tile.tilename = 'DES2359+0001' AND
         (ABS(ima.RA_CENT  -  (tile.RA_CENT-360))  < (0.5*tile.RA_SIZE  + 0.5*ima.RA_SIZE_CCD)) AND
         (ABS(ima.DEC_CENT -  tile.DEC_CENT) < (0.5*tile.DEC_SIZE + 0.5*ima.DEC_SIZE_CCD))
	 order by ima.RA_CENT;

