create table me_inputs_Y2N_FIRSTCUT as
     SELECT 
         file_archive_info.FILENAME,
	 file_archive_info.COMPRESSION,
         file_archive_info.PATH,
         wgb.UNITNAME,wgb.REQNUM,wgb.ATTNUM,
         image.BAND,
         image.CCDNUM,
	 image.RA_CENT,image.DEC_CENT,
         image.RAC1,  image.RAC2,  image.RAC3,  image.RAC4,
         image.DECC1, image.DECC2, image.DECC3, image.DECC4
     FROM
         wgb, ops_proctag, image, file_archive_info
     WHERE
         file_archive_info.FILENAME  = image.FILENAME AND
         file_archive_info.FILENAME  = wgb.FILENAME  AND
         wgb.FILETYPE    = 'red' AND
	 wgb.EXEC_NAME   = 'immask' AND	
         wgb.REQNUM      = ops_proctag.REQNUM AND
         wgb.UNITNAME    = ops_proctag.UNITNAME AND
         wgb.ATTNUM      = ops_proctag.ATTNUM AND
         ops_proctag.TAG = 'Y2N_FIRSTCUT';
