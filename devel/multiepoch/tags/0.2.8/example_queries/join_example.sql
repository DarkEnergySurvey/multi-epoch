SELECT image.FILENAME
     FROM
         (select FILENAME,REQNUM, UNITNAME, ATTNUM from wgb where FILETYPE='red_immask') wgb
     JOIN (select  REQNUM, UNITNAME, ATTNUM from ops_proctag where TAG = 'Y2T3_FINALCUT') ops ON
     wgb.REQNUM    = ops.REQNUM AND
     wgb.UNITNAME  = ops.UNITNAME AND
     wgb.ATTNUM    = ops.ATTNUM 
    JOIN file_archive_info ON file_archive_info.FILENAME  = wgb.FILENAME 
    JOIN image ON file_archive_info.FILENAME  = image.FILENAME; 
--  JOIN felipe.extraZEROPOINT ON felipe.extraZEROPOINT.FILENAME = image.FILENAME;

