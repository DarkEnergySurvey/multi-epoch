    SELECT
	 image.FILENAME,
	 image.RA_CENT,image.DEC_CENT,
         image.BAND
    FROM
	 image, ops_proctag, felipe.coaddtile_new tile
    WHERE
         image.FILETYPE    = 'red_immask' AND
	 image.PFW_ATTEMPT_ID = ops_proctag.PFW_ATTEMPT_ID AND	
--	 Change TAG accordlingly	 
--         ops_proctag.TAG = 'Y2T9_FINALCUT' AND
         ops_proctag.TAG = 'Y2A1_FINALCUT' AND
--       Optionally exclude blacklist
--	 image.filename NOT IN (select filename from image i, GRUENDL.MY_BLACKLIST b where i.expnum=b.expnum and i.ccdnum=b.ccdnum) AND
--	 Change tilename accordingly (examples)
--	 tile.tilename = 'DES2247-4414' AND
	 tile.tilename = 'DES0311-5040' AND	
	 (
          ((image.RAC1 BETWEEN tile.RACMIN AND tile.RACMAX) AND (image.DECC1 BETWEEN tile.DECCMIN AND tile.DECCMAX)) OR
	  ((image.RAC2 BETWEEN tile.RACMIN AND tile.RACMAX) AND (image.DECC2 BETWEEN tile.DECCMIN AND tile.DECCMAX)) OR
	  ((image.RAC3 BETWEEN tile.RACMIN AND tile.RACMAX) AND (image.DECC3 BETWEEN tile.DECCMIN AND tile.DECCMAX)) OR
	  ((image.RAC4 BETWEEN tile.RACMIN AND tile.RACMAX) AND (image.DECC4 BETWEEN tile.DECCMIN AND tile.DECCMAX))
	 )
	 order by image.BAND;

