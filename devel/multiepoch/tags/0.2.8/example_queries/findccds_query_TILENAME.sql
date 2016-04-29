-- Example query to find all CCDS inside a give TILENAME for tag:Y2A1_FINALCUT
-- Case 1: We do not cross RAZERO

     SELECT 
         tile.RA_CENT, tile.DEC_CENT,
         tile.RA_SIZE, tile.DEC_SIZE,
         file_archive_info.FILENAME,
	 file_archive_info.COMPRESSION,
         file_archive_info.PATH,
	 image.PFW_ATTEMPT_ID,
         image.BAND,
         image.CCDNUM,
	 image.RA_CENT,image.DEC_CENT,
         image.RAC1,  image.RAC2,  image.RAC3,  image.RAC4,
         image.DECC1, image.DECC2, image.DECC3, image.DECC4
     FROM
         ops_proctag, image, file_archive_info, felipe.coaddtile_new tile
     WHERE
         file_archive_info.FILENAME  = image.FILENAME AND
         image.FILETYPE    = 'red_immask' AND
	 image.PFW_ATTEMPT_ID = ops_proctag.PFW_ATTEMPT_ID AND
--	 Change tilename accordingly
	 tile.tilename = 'DES0311-5040' AND
--	 tile.tilename = 'DES0309-5205' AND
--	 tile.tilename = 'DES0307-5040' AND
--       tile.tilename = 'DES0306-5123' AND
         (ABS(image.RA_CENT  -  tile.RA_CENT)  < (0.5*tile.RA_SIZE  + 0.5*ABS(image.RAC2 - image.RAC3) )) AND
         (ABS(image.DEC_CENT -  tile.DEC_CENT) < (0.5*tile.DEC_SIZE + 0.5*ABS(image.DECC1- image.DECC2))) AND
         ops_proctag.TAG = 'Y2A1_FINALCUT';
