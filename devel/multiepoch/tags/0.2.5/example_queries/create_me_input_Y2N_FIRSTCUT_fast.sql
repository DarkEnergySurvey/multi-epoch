create table me_images_Y2N_FIRSTCUT as
SELECT fai.filename,
       fai.compression,
       fai.path,
       e.unitname,
       e.reqnum,
       e.attnum,
       i.band,
       i.ccdnum,
       i.ra_cent,
       i.dec_cent,
       i.rac1,
       i.rac2,
       i.rac3,
       i.rac4,
       i.decc1,
       i.decc2,
       i.decc3,
       i.decc4
  FROM file_archive_info fai,
       image i,
       genfile g,
       opm_artifact a,
       opm_was_generated_by wgb,
       pfw_exec e,
       ops_proctag p
 WHERE fai.filename = i.filename
   AND g.filename = fai.filename
   AND g.filename like '%immasked%'
   AND g.filetype = 'red'
   AND a.name = g.filename
   AND wgb.opm_artifact_id = a.id
   AND e.task_id = wgb.task_id
   AND p.reqnum = e.reqnum
   AND p.unitname = e.unitname
   AND p.attnum = e.attnum
   AND p.tag = 'Y2N_FIRSTCUT';
