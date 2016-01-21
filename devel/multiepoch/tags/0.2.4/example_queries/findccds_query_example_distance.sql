    SELECT
         felipe.extraZEROPOINT.MAG_ZERO,
         file_archive_info.FILENAME,file_archive_info.PATH, image.BAND,
         image.RAC1,  image.RAC2,  image.RAC3,  image.RAC4,
         image.DECC1, image.DECC2, image.DECC3, image.DECC4
     FROM
         file_archive_info, wgb, image, ops_proctag,
         felipe.extraZEROPOINT 
     WHERE
         file_archive_info.FILENAME  = image.FILENAME AND
         file_archive_info.FILENAME  = wgb.FILENAME  AND
         image.FILETYPE  = 'red' AND
         wgb.FILETYPE    = 'red' AND
         wgb.EXEC_NAME   = 'immask' AND
         wgb.REQNUM      = ops_proctag.REQNUM AND
         wgb.UNITNAME    = ops_proctag.UNITNAME AND
         wgb.ATTNUM      = ops_proctag.ATTNUM AND
         ops_proctag.TAG = 'Y2T_FIRSTCUT' AND
         felipe.extraZEROPOINT.FILENAME = image.FILENAME AND
	 ABS(image.RA_CENT  - 341.584624919)  < 0.519446 + 0.505*ABS(image.RAC2-image.RAC3 ) AND
	 ABS(image.DEC_CENT - -44.9583333333) < 0.36527  + 0.505*ABS(image.DECC1-image.DECC2 );	
