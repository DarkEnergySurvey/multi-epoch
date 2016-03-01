     SELECT 
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
         ops_proctag, image, file_archive_info
     WHERE
         file_archive_info.FILENAME  = image.FILENAME AND
         image.FILETYPE    = 'red_immask' AND
	 image.PFW_ATTEMPT_ID = ops_proctag.PFW_ATTEMPT_ID AND
         ops_proctag.TAG = 'Y2T9_FINALCUT';
--         ops_proctag.TAG = 'Y2A1_FINALCUT';
