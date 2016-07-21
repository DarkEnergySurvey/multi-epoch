-- Create temporary tables for the CROSSRA0='Y' tables for image and coaddtile
with ima as 
    (SELECT /*+ materialize */ 
         image.FILENAME,
         image.FILETYPE,	
	 image.CROSSRA0,
    	 image.PFW_ATTEMPT_ID,
         image.BAND,
         image.CCDNUM,
	 image.DEC_CENT,
	 (case when image.RA_CENT > 180. THEN image.RA_CENT-360. ELSE image.RA_CENT END) as RA_CENT,
         (case when image.CROSSRA0='Y' THEN abs(image.RACMAX - (image.RACMIN-360)) ELSE abs(image.RACMAX - image.RACMIN) END) as RA_SIZE_CCD,
         abs(image.DECCMAX - image.DECCMIN) as DEC_SIZE_CCD
         FROM image),
tile as 
    (SELECT /*+ materialize */ 
    	 t.tilename,
    	 t.RA_SIZE,
	 t.DEC_SIZE,
	 t.DEC_CENT,
	 (case when (t.CROSSRAZERO='Y' and t.RA_CENT> 180) THEN t.RA_CENT-360. ELSE t.RA_CENT END) as RA_CENT
         FROM felipe.coaddtile_new t)
    SELECT
	 ima.FILENAME,
	 ima.RA_CENT,ima.DEC_CENT,
         ima.BAND,
	 ima.RA_SIZE_CCD,ima.DEC_SIZE_CCD
    FROM
	 ima, ops_proctag, tile
    WHERE
         ima.FILETYPE    = 'red_immask' AND
	 ima.PFW_ATTEMPT_ID = ops_proctag.PFW_ATTEMPT_ID AND	
--	 Change TAG accordlingly	 
         ops_proctag.TAG = 'Y2A1_FINALCUT' AND
--       Optionally exclude blacklist
--	 ima.filename NOT IN (select filename from image i, GRUENDL.MY_BLACKLIST b where i.expnum=b.expnum and i.ccdnum=b.ccdnum) AND
--	 Change tilename accordingly (examples)
	 tile.tilename = 'DES0001-4957' AND
--       tile.tilename = 'DES0516-5457' AND    
--	 tile.tilename = 'DES2359+0043' AND
--	 tile.tilename = 'DES2247-4414' AND
--	 tile.tilename = 'DES2359+0001' AND
--	 tile.tilename = 'DES0002+0043' AND
         (ABS(ima.RA_CENT  -  tile.RA_CENT)  < (0.5*tile.RA_SIZE  + 0.5*ima.RA_SIZE_CCD)) AND
         (ABS(ima.DEC_CENT -  tile.DEC_CENT) < (0.5*tile.DEC_SIZE + 0.5*ima.DEC_SIZE_CCD))
	 order by ima.BAND; > DES0001-4957.tab

