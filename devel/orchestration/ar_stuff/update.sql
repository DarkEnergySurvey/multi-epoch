set pagesize 0 head off term off echo off;
spool out.last;

var cnt number;
exec :cnt := upd_archivesites('&1','&2','&3','&4','&5','&6','&7','&8','&9','&10');
print cnt;

spool off;
exit;
