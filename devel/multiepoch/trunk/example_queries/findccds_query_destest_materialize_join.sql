with ima as 
    (SELECT /*+ materialize */ 
         image.FILENAME,
         image.FILETYPE,	
	 image.CROSSRA0,
    	 image.PFW_ATTEMPT_ID,
         image.BAND,
         image.CCDNUM,
	 image.RA_CENT,image.DEC_CENT,
         (case when image.CROSSRA0='Y' THEN abs(image.RACMAX - (image.RACMIN-360)) ELSE abs(image.RACMAX - image.RACMIN) END) as RA_SIZE_CCD,
         abs(image.DECCMAX - image.DECCMIN) as DEC_SIZE_CCD
         FROM image)
    SELECT
         file_archive_info.FILENAME,
--	 file_archive_info.COMPRESSION,
--       file_archive_info.PATH,
	 ima.RA_CENT,ima.DEC_CENT,
         ima.CROSSRA0,
	 ima.RA_SIZE_CCD,ima.DEC_SIZE_CCD
    FROM
	 ima, ops_proctag, file_archive_info, felipe.coaddtile_new tile
     WHERE
         file_archive_info.FILENAME  = ima.FILENAME AND
         ima.FILETYPE    = 'red_immask' AND
	 ima.PFW_ATTEMPT_ID = ops_proctag.PFW_ATTEMPT_ID AND
--	 Change tilename accordingly
--	 tile.tilename = 'DES0311-5040' AND
--	 tile.tilename = 'DES0309-5205' AND
--	 tile.tilename = 'DES0307-5040' AND
--       tile.tilename = 'DES0306-5123' AND
         tile.tilename = 'DES0516-5457' AND         
         (ABS(ima.RA_CENT  -  tile.RA_CENT)  < (0.5*tile.RA_SIZE  + 0.5*ima.RA_SIZE_CCD)) AND
         (ABS(ima.DEC_CENT -  tile.DEC_CENT) < (0.5*tile.DEC_SIZE + 0.5*ima.DEC_SIZE_CCD))
     order by ima.RA_CENT;
