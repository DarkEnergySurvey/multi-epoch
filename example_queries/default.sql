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
 (((image.RAC1 BETWEEN 341.0651786706 AND 342.1040711675) AND (image.DECC1 BETWEEN -45.3223923637 AND -44.5919493974))
 OR ((image.RAC2 BETWEEN 341.0651786706 AND 342.1040711675) AND (image.DECC2 BETWEEN -45.3223923637 AND -44.5919493974))
 OR ((image.RAC3 BETWEEN 341.0651786706 AND 342.1040711675) AND (image.DECC3 BETWEEN -45.3223923637 AND -44.5919493974))
 OR ((image.RAC4 BETWEEN 341.0651786706 AND 342.1040711675) AND (image.DECC4 BETWEEN -45.3223923637 AND -44.5919493974))
);


