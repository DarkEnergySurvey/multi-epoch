     SELECT 
         distinct 
	 catalog.FILENAME as FILENAME_SCAMPCAT,
	 miscfile.FILENAME as FILENAME_SCAMPHEAD, 
	 file_archive_info.PATH,
	 catalog.expnum, catalog.BAND
     FROM 
         file_archive_info,
         miscfile,
     	 ops_proctag,	
         catalog
     WHERE
         file_archive_info.FILENAME  = catalog.FILENAME AND	
	 catalog.PFW_ATTEMPT_ID  = ops_proctag.PFW_ATTEMPT_ID AND
	 miscfile.PFW_ATTEMPT_ID = ops_proctag.PFW_ATTEMPT_ID AND
	 miscfile.FILETYPE = 'head_scamp_full' AND
         catalog.FILETYPE  = 'cat_scamp_full' AND
	 miscfile.EXPNUM   = catalog.EXPNUM AND
         ops_proctag.TAG = 'Y2T9_FINALCUT'
         order by catalog.BAND;
