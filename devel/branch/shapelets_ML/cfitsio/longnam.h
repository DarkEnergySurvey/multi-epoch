#ifndef _LONGNAME_H
#define _LONGNAME_H

#define fits_parse_input_url ffiurl
#define fits_parse_input_filename ffifile
#define fits_parse_rootname ffrtnm
#define fits_file_exists    ffexist
#define fits_parse_output_url ffourl
#define fits_parse_extspec  ffexts
#define fits_parse_extnum   ffextn
#define fits_parse_binspec  ffbins
#define fits_parse_binrange ffbinr
#define fits_parse_range    ffrwrg
#define fits_parse_rangell    ffrwrgll
#define fits_open_memfile   ffomem

/* 
   use the following special macro to test that the fitsio.h include
   file that was used to build the CFITSIO library is the same version
   as included when compiling the application program
*/
#define fits_open_file(A, B, C, D)  ffopentest( CFITSIO_VERSION, A, B, C, D)

#define fits_open_data      ffdopn
#define fits_open_table     fftopn
#define fits_open_image     ffiopn
#define fits_open_diskfile  ffdkopn
#define fits_reopen_file    ffreopen
#define fits_create_file    ffinit
#define fits_create_diskfile ffdkinit
#define fits_create_memfile ffimem
#define fits_create_template fftplt
#define fits_flush_file     ffflus
#define fits_flush_buffer   ffflsh
#define fits_close_file     ffclos
#define fits_delete_file    ffdelt
#define fits_file_name      ffflnm
#define fits_file_mode      ffflmd
#define fits_url_type       ffurlt

#define fits_get_version    ffvers
#define fits_uppercase      ffupch
#define fits_get_errstatus  ffgerr
#define fits_write_errmsg   ffpmsg
#define fits_write_errmark  ffpmrk
#define fits_read_errmsg    ffgmsg
#define fits_clear_errmsg   ffcmsg
#define fits_clear_errmark  ffcmrk
#define fits_report_error   ffrprt
#define fits_compare_str    ffcmps
#define fits_test_keyword   fftkey
#define fits_test_record    fftrec
#define fits_null_check     ffnchk
#define fits_make_keyn      ffkeyn
#define fits_make_nkey      ffnkey
#define fits_get_keyclass   ffgkcl
#define fits_get_keytype    ffdtyp
#define fits_parse_value    ffpsvc
#define fits_get_keyname    ffgknm
#define fits_parse_template ffgthd
#define fits_ascii_tform    ffasfm
#define fits_binary_tform   ffbnfm
#define fits_binary_tformll   ffbnfmll
#define fits_get_tbcol      ffgabc
#define fits_get_rowsize    ffgrsz
#define fits_get_col_display_width    ffgcdw

#define fits_write_record       ffprec
#define fits_write_key          ffpky
#define fits_write_key_unit     ffpunt
#define fits_write_comment      ffpcom
#define fits_write_history      ffphis 
#define fits_write_date         ffpdat
#define fits_get_system_time    ffgstm
#define fits_get_system_date    ffgsdt
#define fits_date2str           ffdt2s
#define fits_time2str           fftm2s
#define fits_str2date           ffs2dt
#define fits_str2time           ffs2tm
#define fits_write_key_longstr  ffpkls
#define fits_write_key_longwarn ffplsw
#define fits_write_key_null     ffpkyu
#define fits_write_key_str      ffpkys
#define fits_write_key_log      ffpkyl
#define fits_write_key_lng      ffpkyj
#define fits_write_key_fixflt   ffpkyf
#define fits_write_key_flt      ffpkye
#define fits_write_key_fixdbl   ffpkyg
#define fits_write_key_dbl      ffpkyd
#define fits_write_key_fixcmp   ffpkfc
#define fits_write_key_cmp      ffpkyc
#define fits_write_key_fixdblcmp ffpkfm
#define fits_write_key_dblcmp   ffpkym
#define fits_write_key_triple   ffpkyt
#define fits_write_tdim         ffptdm
#define fits_write_tdimll       ffptdmll
#define fits_write_keys_str     ffpkns
#define fits_write_keys_log     ffpknl
#define fits_write_keys_lng     ffpknj
#define fits_write_keys_fixflt  ffpknf
#define fits_write_keys_flt     ffpkne
#define fits_write_keys_fixdbl  ffpkng
#define fits_write_keys_dbl     ffpknd
#define fits_copy_key           ffcpky
#define fits_write_imghdr       ffphps
#define fits_write_imghdrll     ffphpsll
#define fits_write_grphdr       ffphpr
#define fits_write_grphdrll     ffphprll
#define fits_write_atblhdr      ffphtb
#define fits_write_btblhdr      ffphbn
#define fits_write_key_template ffpktp

#define fits_get_hdrspace      ffghsp
#define fits_get_hdrpos        ffghps
#define fits_movabs_key        ffmaky
#define fits_movrel_key        ffmrky
#define fits_find_nextkey      ffgnxk

#define fits_read_record       ffgrec
#define fits_read_card         ffgcrd
#define fits_read_key_unit     ffgunt
#define fits_read_keyn         ffgkyn
#define fits_read_key          ffgky
#define fits_read_keyword      ffgkey
#define fits_read_key_str      ffgkys
#define fits_read_key_log      ffgkyl
#define fits_read_key_lng      ffgkyj
#define fits_read_key_lnglng      ffgkyjj
#define fits_read_key_flt      ffgkye
#define fits_read_key_dbl      ffgkyd
#define fits_read_key_cmp      ffgkyc
#define fits_read_key_dblcmp   ffgkym
#define fits_read_key_triple   ffgkyt
#define fits_read_key_longstr  ffgkls
#define fits_read_tdim         ffgtdm
#define fits_read_tdimll       ffgtdmll
#define fits_decode_tdim       ffdtdm
#define fits_decode_tdimll     ffdtdmll
#define fits_read_keys_str     ffgkns
#define fits_read_keys_log     ffgknl
#define fits_read_keys_lng     ffgknj
#define fits_read_keys_flt     ffgkne
#define fits_read_keys_dbl     ffgknd
#define fits_read_imghdr       ffghpr
#define fits_read_imghdrll     ffghprll
#define fits_read_atblhdr      ffghtb
#define fits_read_btblhdr      ffghbn
#define fits_read_atblhdrll      ffghtbll
#define fits_read_btblhdrll      ffghbnll
#define fits_hdr2str           ffhdr2str

#define fits_update_card       ffucrd
#define fits_update_key        ffuky
#define fits_update_key_null   ffukyu
#define fits_update_key_str    ffukys
#define fits_update_key_longstr    ffukls
#define fits_update_key_log    ffukyl
#define fits_update_key_lng    ffukyj
#define fits_update_key_fixflt ffukyf
#define fits_update_key_flt    ffukye
#define fits_update_key_fixdbl ffukyg
#define fits_update_key_dbl    ffukyd
#define fits_update_key_fixcmp ffukfc
#define fits_update_key_cmp    ffukyc
#define fits_update_key_fixdblcmp ffukfm
#define fits_update_key_dblcmp ffukym

#define fits_modify_record     ffmrec
#define fits_modify_card       ffmcrd
#define fits_modify_name       ffmnam
#define fits_modify_comment    ffmcom
#define fits_modify_key_null   ffmkyu
#define fits_modify_key_str    ffmkys
#define fits_modify_key_longstr    ffmkls
#define fits_modify_key_log    ffmkyl
#define fits_modify_key_lng    ffmkyj
#define fits_modify_key_fixflt ffmkyf
#define fits_modify_key_flt    ffmkye
#define fits_modify_key_fixdbl ffmkyg
#define fits_modify_key_dbl    ffmkyd
#define fits_modify_key_fixcmp ffmkfc
#define fits_modify_key_cmp    ffmkyc
#define fits_modify_key_fixdblcmp ffmkfm
#define fits_modify_key_dblcmp ffmkym

#define fits_insert_record     ffirec
#define fits_insert_card       ffikey
#define fits_insert_key_null   ffikyu
#define fits_insert_key_str    ffikys
#define fits_insert_key_longstr    ffikls
#define fits_insert_key_log    ffikyl
#define fits_insert_key_lng    ffikyj
#define fits_insert_key_fixflt ffikyf
#define fits_insert_key_flt    ffikye
#define fits_insert_key_fixdbl ffikyg
#define fits_insert_key_dbl    ffikyd
#define fits_insert_key_fixcmp ffikfc
#define fits_insert_key_cmp    ffikyc
#define fits_insert_key_fixdblcmp ffikfm
#define fits_insert_key_dblcmp ffikym

#define fits_delete_key     ffdkey
#define fits_delete_record  ffdrec
#define fits_get_hdu_num    ffghdn
#define fits_get_hdu_type   ffghdt
#define fits_get_hduaddr    ffghad
#define fits_get_hduaddrll    ffghadll
#define fits_get_hduoff     ffghof

#define fits_get_img_param  ffgipr
#define fits_get_img_paramll  ffgiprll

#define fits_get_img_type   ffgidt
#define fits_get_img_equivtype   ffgiet
#define fits_get_img_dim    ffgidm
#define fits_get_img_size   ffgisz
#define fits_get_img_sizell   ffgiszll

#define fits_movabs_hdu     ffmahd
#define fits_movrel_hdu     ffmrhd
#define fits_movnam_hdu     ffmnhd
#define fits_get_num_hdus   ffthdu
#define fits_create_img     ffcrim
#define fits_create_imgll   ffcrimll
#define fits_create_tbl     ffcrtb
#define fits_create_hdu     ffcrhd
#define fits_insert_img     ffiimg
#define fits_insert_imgll   ffiimgll
#define fits_insert_atbl    ffitab
#define fits_insert_btbl    ffibin
#define fits_resize_img     ffrsim
#define fits_resize_imgll   ffrsimll

#define fits_delete_hdu     ffdhdu
#define fits_copy_hdu       ffcopy
#define fits_copy_file      ffcpfl
#define fits_copy_header    ffcphd
#define fits_copy_data      ffcpdt

#define fits_set_hdustruc   ffrdef
#define fits_set_hdrsize    ffhdef
#define fits_write_theap    ffpthp

#define fits_encode_chksum  ffesum
#define fits_decode_chksum  ffdsum
#define fits_write_chksum   ffpcks
#define fits_update_chksum  ffupck
#define fits_verify_chksum  ffvcks
#define fits_get_chksum     ffgcks

#define fits_set_bscale     ffpscl
#define fits_set_tscale     fftscl
#define fits_set_imgnull    ffpnul
#define fits_set_btblnull   fftnul
#define fits_set_atblnull   ffsnul

#define fits_get_colnum     ffgcno
#define fits_get_colname    ffgcnn
#define fits_get_coltype    ffgtcl
#define fits_get_coltypell  ffgtclll
#define fits_get_eqcoltype  ffeqty
#define fits_get_eqcoltypell ffeqtyll
#define fits_get_num_rows   ffgnrw
#define fits_get_num_rowsll   ffgnrwll
#define fits_get_num_cols   ffgncl
#define fits_get_acolparms  ffgacl
#define fits_get_bcolparms  ffgbcl
#define fits_get_bcolparmsll  ffgbclll

#define fits_iterate_data   ffiter

#define fits_read_grppar_byt  ffggpb
#define fits_read_grppar_sbyt  ffggpsb
#define fits_read_grppar_usht  ffggpui
#define fits_read_grppar_ulng  ffggpuj
#define fits_read_grppar_sht  ffggpi
#define fits_read_grppar_lng  ffggpj
#define fits_read_grppar_lnglng  ffggpjj
#define fits_read_grppar_int  ffggpk
#define fits_read_grppar_uint  ffggpuk
#define fits_read_grppar_flt  ffggpe
#define fits_read_grppar_dbl  ffggpd

#define fits_read_pix         ffgpxv
#define fits_read_pixll       ffgpxvll
#define fits_read_pixnull     ffgpxf
#define fits_read_pixnullll   ffgpxfll
#define fits_read_img         ffgpv
#define fits_read_imgnull     ffgpf
#define fits_read_img_byt     ffgpvb
#define fits_read_img_sbyt     ffgpvsb
#define fits_read_img_usht     ffgpvui
#define fits_read_img_ulng     ffgpvuj
#define fits_read_img_sht     ffgpvi
#define fits_read_img_lng     ffgpvj
#define fits_read_img_lnglng     ffgpvjj
#define fits_read_img_uint     ffgpvuk
#define fits_read_img_int     ffgpvk
#define fits_read_img_flt     ffgpve
#define fits_read_img_dbl     ffgpvd

#define fits_read_imgnull_byt ffgpfb
#define fits_read_imgnull_sbyt ffgpfsb
#define fits_read_imgnull_usht ffgpfui
#define fits_read_imgnull_ulng ffgpfuj
#define fits_read_imgnull_sht ffgpfi
#define fits_read_imgnull_lng ffgpfj
#define fits_read_imgnull_lnglng ffgpfjj
#define fits_read_imgnull_uint ffgpfuk
#define fits_read_imgnull_int ffgpfk
#define fits_read_imgnull_flt ffgpfe
#define fits_read_imgnull_dbl ffgpfd

#define fits_read_2d_byt      ffg2db
#define fits_read_2d_sbyt     ffg2dsb
#define fits_read_2d_usht      ffg2dui
#define fits_read_2d_ulng      ffg2duj
#define fits_read_2d_sht      ffg2di
#define fits_read_2d_lng      ffg2dj
#define fits_read_2d_lnglng      ffg2djj
#define fits_read_2d_uint      ffg2duk
#define fits_read_2d_int      ffg2dk
#define fits_read_2d_flt      ffg2de
#define fits_read_2d_dbl      ffg2dd

#define fits_read_3d_byt      ffg3db
#define fits_read_3d_sbyt      ffg3dsb
#define fits_read_3d_usht      ffg3dui
#define fits_read_3d_ulng      ffg3duj
#define fits_read_3d_sht      ffg3di
#define fits_read_3d_lng      ffg3dj
#define fits_read_3d_lnglng      ffg3djj
#define fits_read_3d_uint      ffg3duk
#define fits_read_3d_int      ffg3dk
#define fits_read_3d_flt      ffg3de
#define fits_read_3d_dbl      ffg3dd

#define fits_read_subset      ffgsv
#define fits_read_subset_byt  ffgsvb
#define fits_read_subset_sbyt  ffgsvsb
#define fits_read_subset_usht  ffgsvui
#define fits_read_subset_ulng  ffgsvuj
#define fits_read_subset_sht  ffgsvi
#define fits_read_subset_lng  ffgsvj
#define fits_read_subset_lnglng  ffgsvjj
#define fits_read_subset_uint  ffgsvuk
#define fits_read_subset_int  ffgsvk
#define fits_read_subset_flt  ffgsve
#define fits_read_subset_dbl  ffgsvd

#define fits_read_subsetnull_byt ffgsfb
#define fits_read_subsetnull_sbyt ffgsfsb
#define fits_read_subsetnull_usht ffgsfui
#define fits_read_subsetnull_ulng ffgsfuj
#define fits_read_subsetnull_sht ffgsfi
#define fits_read_subsetnull_lng ffgsfj
#define fits_read_subsetnull_lnglng ffgsfjj
#define fits_read_subsetnull_uint ffgsfuk
#define fits_read_subsetnull_int ffgsfk
#define fits_read_subsetnull_flt ffgsfe
#define fits_read_subsetnull_dbl ffgsfd

#define fits_compress_img fits_comp_img
#define fits_decompress_img fits_decomp_img

#define fits_read_col        ffgcv
#define fits_read_colnull    ffgcf
#define fits_read_col_str    ffgcvs
#define fits_read_col_log    ffgcvl
#define fits_read_col_byt    ffgcvb
#define fits_read_col_sbyt    ffgcvsb
#define fits_read_col_usht    ffgcvui
#define fits_read_col_ulng    ffgcvuj
#define fits_read_col_sht    ffgcvi
#define fits_read_col_lng    ffgcvj
#define fits_read_col_lnglng    ffgcvjj
#define fits_read_col_uint    ffgcvuk
#define fits_read_col_int    ffgcvk
#define fits_read_col_flt    ffgcve
#define fits_read_col_dbl    ffgcvd
#define fits_read_col_cmp    ffgcvc
#define fits_read_col_dblcmp ffgcvm
#define fits_read_col_bit    ffgcx
#define fits_read_col_bit_usht ffgcxui
#define fits_read_col_bit_uint ffgcxuk

#define fits_read_colnull_str    ffgcfs
#define fits_read_colnull_log    ffgcfl
#define fits_read_colnull_byt    ffgcfb
#define fits_read_colnull_sbyt    ffgcfsb
#define fits_read_colnull_usht    ffgcfui
#define fits_read_colnull_ulng    ffgcfuj
#define fits_read_colnull_sht    ffgcfi
#define fits_read_colnull_lng    ffgcfj
#define fits_read_colnull_lnglng    ffgcfjj
#define fits_read_colnull_uint    ffgcfuk
#define fits_read_colnull_int    ffgcfk
#define fits_read_colnull_flt    ffgcfe
#define fits_read_colnull_dbl    ffgcfd
#define fits_read_colnull_cmp    ffgcfc
#define fits_read_colnull_dblcmp ffgcfm

#define fits_read_descript ffgdes
#define fits_read_descriptll ffgdesll
#define fits_read_descripts ffgdess
#define fits_read_descriptsll ffgdessll
#define fits_read_tblbytes    ffgtbb

#define fits_write_grppar_byt ffpgpb
#define fits_write_grppar_sbyt ffpgpsb
#define fits_write_grppar_usht ffpgpui
#define fits_write_grppar_ulng ffpgpuj
#define fits_write_grppar_sht ffpgpi
#define fits_write_grppar_lng ffpgpj
#define fits_write_grppar_lnglng ffpgpjj
#define fits_write_grppar_uint ffpgpuk
#define fits_write_grppar_int ffpgpk
#define fits_write_grppar_flt ffpgpe
#define fits_write_grppar_dbl ffpgpd

#define fits_write_pix        ffppx
#define fits_write_pixll      ffppxll
#define fits_write_pixnull    ffppxn
#define fits_write_pixnullll  ffppxnll
#define fits_write_img        ffppr
#define fits_write_img_byt    ffpprb
#define fits_write_img_sbyt    ffpprsb
#define fits_write_img_usht    ffpprui
#define fits_write_img_ulng    ffppruj
#define fits_write_img_sht    ffppri
#define fits_write_img_lng    ffpprj
#define fits_write_img_lnglng    ffpprjj
#define fits_write_img_uint    ffppruk
#define fits_write_img_int    ffpprk
#define fits_write_img_flt    ffppre
#define fits_write_img_dbl    ffpprd

#define fits_write_imgnull     ffppn
#define fits_write_imgnull_byt ffppnb
#define fits_write_imgnull_sbyt ffppnsb
#define fits_write_imgnull_usht ffppnui
#define fits_write_imgnull_ulng ffppnuj
#define fits_write_imgnull_sht ffppni
#define fits_write_imgnull_lng ffppnj
#define fits_write_imgnull_lnglng ffppnjj
#define fits_write_imgnull_uint ffppnuk
#define fits_write_imgnull_int ffppnk
#define fits_write_imgnull_flt ffppne
#define fits_write_imgnull_dbl ffppnd

#define fits_write_img_null ffppru
#define fits_write_null_img ffpprn

#define fits_write_2d_byt   ffp2db
#define fits_write_2d_sbyt   ffp2dsb
#define fits_write_2d_usht   ffp2dui
#define fits_write_2d_ulng   ffp2duj
#define fits_write_2d_sht   ffp2di
#define fits_write_2d_lng   ffp2dj
#define fits_write_2d_lnglng   ffp2djj
#define fits_write_2d_uint   ffp2duk
#define fits_write_2d_int   ffp2dk
#define fits_write_2d_flt   ffp2de
#define fits_write_2d_dbl   ffp2dd

#define fits_write_3d_byt   ffp3db
#define fits_write_3d_sbyt   ffp3dsb
#define fits_write_3d_usht   ffp3dui
#define fits_write_3d_ulng   ffp3duj
#define fits_write_3d_sht   ffp3di
#define fits_write_3d_lng   ffp3dj
#define fits_write_3d_lnglng   ffp3djj
#define fits_write_3d_uint   ffp3duk
#define fits_write_3d_int   ffp3dk
#define fits_write_3d_flt   ffp3de
#define fits_write_3d_dbl   ffp3dd

#define fits_write_subset  ffpss
#define fits_write_subset_byt  ffpssb
#define fits_write_subset_sbyt  ffpsssb
#define fits_write_subset_usht  ffpssui
#define fits_write_subset_ulng  ffpssuj
#define fits_write_subset_sht  ffpssi
#define fits_write_subset_lng  ffpssj
#define fits_write_subset_lnglng  ffpssjj
#define fits_write_subset_uint  ffpssuk
#define fits_write_subset_int  ffpssk
#define fits_write_subset_flt  ffpsse
#define fits_write_subset_dbl  ffpssd

#define fits_write_col         ffpcl
#define fits_write_col_str     ffpcls
#define fits_write_col_log     ffpcll
#define fits_write_col_byt     ffpclb
#define fits_write_col_sbyt     ffpclsb
#define fits_write_col_usht     ffpclui
#define fits_write_col_ulng     ffpcluj
#define fits_write_col_sht     ffpcli
#define fits_write_col_lng     ffpclj
#define fits_write_col_lnglng     ffpcljj
#define fits_write_col_uint     ffpcluk
#define fits_write_col_int     ffpclk
#define fits_write_col_flt     ffpcle
#define fits_write_col_dbl     ffpcld
#define fits_write_col_cmp     ffpclc
#define fits_write_col_dblcmp  ffpclm
#define fits_write_col_null    ffpclu
#define fits_write_col_bit     ffpclx
#define fits_write_nulrows     ffprwu

#define fits_write_colnull ffpcn
#define fits_write_colnull_str ffpcns
#define fits_write_colnull_log ffpcnl
#define fits_write_colnull_byt ffpcnb
#define fits_write_colnull_sbyt ffpcnsb
#define fits_write_colnull_usht ffpcnui
#define fits_write_colnull_ulng ffpcnuj
#define fits_write_colnull_sht ffpcni
#define fits_write_colnull_lng ffpcnj
#define fits_write_colnull_lnglng ffpcnjj
#define fits_write_colnull_uint ffpcnuk
#define fits_write_colnull_int ffpcnk
#define fits_write_colnull_flt ffpcne
#define fits_write_colnull_dbl ffpcnd

#define fits_write_descript  ffpdes
#define fits_compress_heap   ffcmph
#define fits_test_heap   fftheap

#define fits_write_tblbytes  ffptbb
#define fits_insert_rows  ffirow
#define fits_delete_rows  ffdrow
#define fits_delete_rowrange ffdrrg
#define fits_delete_rowlist ffdrws
#define fits_delete_rowlistll ffdrwsll
#define fits_insert_col   fficol
#define fits_insert_cols  fficls
#define fits_delete_col   ffdcol
#define fits_copy_col     ffcpcl
#define fits_modify_vector_len  ffmvec

#define fits_read_img_coord ffgics
#define fits_read_tbl_coord ffgtcs
#define fits_pix_to_world ffwldp
#define fits_world_to_pix ffxypx

#define fits_get_image_wcs_keys ffgiwcs
#define fits_get_table_wcs_keys ffgtwcs

#define fits_find_rows          fffrow
#define fits_find_first_row     ffffrw
#define fits_find_rows_cmp      fffrwc
#define fits_select_rows        ffsrow
#define fits_calc_rows          ffcrow
#define fits_calculator         ffcalc
#define fits_calculator_rng     ffcalc_rng
#define fits_test_expr          fftexp

#define fits_create_group       ffgtcr 
#define fits_insert_group       ffgtis 
#define fits_change_group       ffgtch 
#define fits_remove_group       ffgtrm 
#define fits_copy_group         ffgtcp 
#define fits_merge_groups       ffgtmg 
#define fits_compact_group      ffgtcm 
#define fits_verify_group       ffgtvf 
#define fits_open_group         ffgtop 
#define fits_add_group_member   ffgtam 
#define fits_get_num_members    ffgtnm 

#define fits_get_num_groups     ffgmng 
#define fits_open_member        ffgmop 
#define fits_copy_member        ffgmcp 
#define fits_transfer_member    ffgmtf 
#define fits_remove_member      ffgmrm

#endif
