SET TRIMOUT   ON
SET HEADING   OFF


-- select trim(BOTH FROM LOCATION_NAME) from archive_sites;
-- select LOCATION_ID,LOCATION_NAME,ARCHIVE_HOST,ARCHIVE_ROOT,STAGING_HOST,STAGING_ROOT,IP_SUBNET,DOWNLOAD_DIR from archive_sites;
-- describe archive_sites;

-- select * from archive_sites;
-- select trim(BOTH FROM LOCATION_NAME) from archive_sites;
-- select trim(BOTH FROM LOCATION_ID) from archive_sites;
-- select trim(BOTH FROM ARCHIVE_HOST) from archive_sites;
-- select trim(BOTH FROM ARCHIVE_ROOT) from archive_sites;
-- select trim(BOTH FROM STAGING_HOST) from archive_sites;
-- select trim(BOTH FROM STAGING_ROOT) from archive_sites;
-- select trim(BOTH FROM IP_SUBNET) from archive_sites;
-- select trim(BOTH FROM DOWNLOAD_DIR) from archive_sites;

-- select LOCATION_NAME from archive_sites;
-- select LOCATION_ID from archive_sites;
-- select ARCHIVE_HOST from archive_sites;
-- select ARCHIVE_ROOT from archive_sites;
-- select STAGING_HOST from archive_sites;
-- select STAGING_ROOT from archive_sites;
-- select IP_SUBNET from archive_sites;
-- select DOWNLOAD_DIR from archive_sites;
-- select * from archive_sites where ip_subnet = 131;
-- select * from archive_sites;
-- select LOCATION_ID,LOCATION_NAME,ARCHIVE_HOST,ARCHIVE_ROOT,STAGING_HOST,STAGING_ROOT,IP_SUBNET,DOWNLOAD_DIR from archive_sites where ip_subnet = 131;

-- select * from table(get_srclist('bcs051119'));
select * from table(get_srclist('&1')) where ARCHIVESITE = '&2';

quit;
