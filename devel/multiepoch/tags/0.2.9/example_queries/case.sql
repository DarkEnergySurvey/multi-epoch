SELECT 
       me.FILENAME,me.COMPRESSION,me.PATH,me.BAND,
       (case when me.RA_CENT > 180 THEN me.RA_CENT-360 ELSE me.RA_CENT END) as RA_CENT, 
       (case when me.RAC1 > 180    THEN me.RAC1-360 ELSE me.RAC1 END) as RAC1,	  
       (case when me.RAC2 > 180    THEN me.RAC2-360 ELSE me.RAC2 END) as RAC2,		
       (case when me.RAC3 > 180    THEN me.RAC3-360 ELSE me.RAC3 END) as RAC3,
       (case when me.RAC4 > 180    THEN me.RAC4-360 ELSE me.RAC4 END) as RAC4,
--     The RA and DEC size of the CCDS
       ABS(me.RAC2  - me.RAC3 )  as RA_SIZE_CCD,
       ABS(me.DECC1 - me.DECC2 ) as DEC_SIZE_CCD,	
--     Change to use RACMIN,DECCMAX once they are computed
--     ABS(me.RACMIN  - me.RACMAX )  as RA_SIZE_CCDM,
--     ABS(me.DECCMIN - me.DECCMAC ) as DEC_SIZE_CCDM,	
       me.DEC_CENT, me.DECC1, me.DECC2, me.DECC3, me.DECC4			
       from felipe.me_inputs_Y2T4_FINALCUT me;
