/*
   Copyright (C) 2002 by Pascal Bauermeister

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with PocketBee; see the file COPYING.  If not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Pascal Bauermeister
   Contributors:

   $Id$
*/

#ifndef __LIBPG_DIRVIEW_MIME_H__
#define __LIBPG_DIRVIEW_MIME_H__

/*!
 * \defgroup enum LpgdvMimeTypeId
 *
 * Internal numbers to identify mime types within the library
 * 
 * \sa lpgdv_mime_type_of
 * 
 * \{
 */
typedef enum {
  MIME_UNKNOWN = -1,

  /* with double extension */
  MIME_application__x_bzip_compressed_tar,
  MIME_application__x_compressed_tar,
  MIME_application__x_cpio_compressed,
  MIME_application__x_profile,
  MIME_image__x_compressed_xcf,

  /* last extension matters */
  MIME_application__andrew_inset = 0,
  MIME_application__msword,
  MIME_application__octet_stream,
  MIME_application__oda,
  MIME_application__pdf,
  MIME_application__pgp,
  MIME_application__postscript,
  MIME_application__qif,
  MIME_application__rtf,
  MIME_application__vnd_lotus_1_2_3,
  MIME_application__vnd_ms_excel,
  MIME_application__vnd_ms_powerpoint,
  MIME_application__vnd_stardivision_calc,
  MIME_application__vnd_stardivision_chart,
  MIME_application__vnd_stardivision_draw,
  MIME_application__vnd_stardivision_impress,
  MIME_application__vnd_stardivision_mail,
  MIME_application__vnd_stardivision_math,
  MIME_application__vnd_stardivision_writer,
  MIME_application__vnd_sun_xml_calc,
  MIME_application__vnd_sun_xml_calc_template,
  MIME_application__vnd_sun_xml_draw,
  MIME_application__vnd_sun_xml_draw_template,
  MIME_application__vnd_sun_xml_impress,
  MIME_application__vnd_sun_xml_impress_template,
  MIME_application__vnd_sun_xml_math,
  MIME_application__vnd_sun_xml_writer,
  MIME_application__vnd_sun_xml_writer_global,
  MIME_application__vnd_sun_xml_writer_template,
  MIME_application__x_abiword,
  MIME_application__x_applix_spreadsheet,
  MIME_application__x_applix_word,
  MIME_application__x_arj,
  MIME_application__x_asp,
  MIME_application__x_backup,
  MIME_application__x_bcpio,
  MIME_application__x_bzip,
  MIME_application__x_cgi,
  MIME_application__x_chess_pgn,
  MIME_application__x_compress,
  MIME_application__x_core_file,
  MIME_application__x_cpio,
  MIME_application__x_dc_rom,
  MIME_application__x_deb,
  MIME_application__x_dia_diagram,
  MIME_application__x_dvi,
  MIME_application__x_e_theme,
  MIME_application__x_font_afm,
  MIME_application__x_font_bdf,
  MIME_application__x_font_linux_psf,
  MIME_application__x_font_pcf,
  MIME_application__x_font_speedo,
  MIME_application__x_font_ttf,
  MIME_application__x_font_type1,
  MIME_application__x_gameboy_rom,
  MIME_application__x_genesis_rom,
  MIME_application__x_glade,
  MIME_application__x_gnome_app_info,
  MIME_application__x_gnucash,
  MIME_application__x_gnumeric,
  MIME_application__x_gtar,
  MIME_application__x_gzip,
  MIME_application__x_hdf,
  MIME_application__x_java_byte_code,
  MIME_application__x_jbuilder_project,
  MIME_application__x_kde_app_info,
  MIME_application__x_killustrator,
  MIME_application__x_kpresenter,
  MIME_application__x_kspread,
  MIME_application__x_kword,
  MIME_application__x_lha,
  MIME_application__x_lhz,
  MIME_application__x_mif,
  MIME_application__x_mrproject,
  MIME_application__x_ms_dos_executable,
  MIME_application__x_msx_rom,
  MIME_application__x_n64_rom,
  MIME_application__x_nes_rom,
  MIME_application__x_netcdf,
  MIME_application__x_object_file,
  MIME_application__x_ogg,
  MIME_application__x_oleo,
  MIME_application__x_php,
  MIME_application__x_python_byte_code,
  MIME_application__x_qw,
  MIME_application__x_rar,
  MIME_application__x_rar_compressed,
  MIME_application__x_reject,
  MIME_application__x_rpm,
  MIME_application__x_shar,
  MIME_application__x_shared_library,
  MIME_application__x_shared_library_la,
  MIME_application__x_shockwave_flash,
  MIME_application__x_smil,
  MIME_application__x_sms_rom,
  MIME_application__x_sv4cpio,
  MIME_application__x_sv4crc,
  MIME_application__x_tar,
  MIME_application__x_theme,
  MIME_application__x_troff_man_compressed,
  MIME_application__x_unix_archive,
  MIME_application__x_ustar,
  MIME_application__x_wais_source,
  MIME_application__x_xbase,
  MIME_application__x_zoo,
  MIME_application__zip,
  MIME_audio__ac3,
  MIME_audio__basic,
  MIME_audio__prs_sid,
  MIME_audio__x_aiff,
  MIME_audio__x_it,
  MIME_audio__x_midi,
  MIME_audio__x_mod,
  MIME_audio__x_mp3,
  MIME_audio__x_mpegurl,
  MIME_audio__x_real_audio,
  MIME_audio__x_s3m,
  MIME_audio__x_stm,
  MIME_audio__x_ulaw,
  MIME_audio__x_voc,
  MIME_audio__x_wav,
  MIME_audio__x_xi,
  MIME_audio__x_xm,
  MIME_image__bmp,
  MIME_image__cgm,
  MIME_image__gif,
  MIME_image__ief,
  MIME_image__jpeg,
  MIME_image__png,
  MIME_image__svg,
  MIME_image__tiff,
  MIME_image__vnd_dwg,
  MIME_image__vnd_dxf,
  MIME_image__x_3ds,
  MIME_image__x_applix_graphic,
  MIME_image__x_bmp,
  MIME_image__x_cmu_raster,
  MIME_image__x_emf,
  MIME_image__x_ico,
  MIME_image__x_iff,
  MIME_image__x_ilbm,
  MIME_image__x_lwo,
  MIME_image__x_lws,
  MIME_image__x_pcx,
  MIME_image__x_photo_cd,
  MIME_image__x_pict,
  MIME_image__x_portable_anymap,
  MIME_image__x_portable_bitmap,
  MIME_image__x_portable_graymap,
  MIME_image__x_portable_pixmap,
  MIME_image__x_psd,
  MIME_image__x_rgb,
  MIME_image__x_tga,
  MIME_image__x_wmf,
  MIME_image__x_xbitmap,
  MIME_image__x_xcf,
  MIME_image__x_xfig,
  MIME_image__x_xpixmap,
  MIME_image__x_xwindowdump,
  MIME_message__x_gnu_rmail,
  MIME_model__vrml,
  MIME_text__abiword,
  MIME_text__bib,
  MIME_text__calendar,
  MIME_text__css,
  MIME_text__html,
  MIME_text__mathml,
  MIME_text__plain,
  MIME_text__richtext,
  MIME_text__sgml,
  MIME_text__spreadsheet,
  MIME_text__tab_separated_values,
  MIME_text__x_authors,
  MIME_text__x_c,
  MIME_text__x_cpp,
  MIME_text__x_c_header,
  MIME_text__x_comma_separated_values,
  MIME_text__x_copying,
  MIME_text__x_credits,
  MIME_text__x_csh,
  MIME_text__x_dcl,
  MIME_text__x_dsl,
  MIME_text__x_dtd,
  MIME_text__x_emacs_lisp,
  MIME_text__x_fortran,
  MIME_text__x_gtkrc,
  MIME_text__x_haskell,
  MIME_text__x_idl,
  MIME_text__x_install,
  MIME_text__x_java,
  MIME_text__x_literate_haskell,
  MIME_text__x_lyx,
  MIME_text__x_makefile,
  MIME_text__x_perl,
  MIME_text__x_python,
  MIME_text__x_readme,
  MIME_text__x_scheme,
  MIME_text__x_setext,
  MIME_text__x_sh,
  MIME_text__x_sql,
  MIME_text__x_tcl,
  MIME_text__x_tex,
  MIME_text__x_texinfo,
  MIME_text__x_troff,
  MIME_text__x_troff_man,
  MIME_text__x_troff_me,
  MIME_text__x_troff_mm,
  MIME_text__x_troff_ms,
  MIME_text__x_vcalendar,
  MIME_text__x_vcard,
  MIME_text__xml,
  MIME_video__mpeg,
  MIME_video__quicktime,
  MIME_video__x_anim,
  MIME_video__x_flc,
  MIME_video__x_fli,
  MIME_video__x_ms_asf,
  MIME_video__x_ms_wmv,
  MIME_video__x_msvideo,
  MIME_video__x_sgi_movie,
} LpgdvMimeTypeId;
//! \}

/*
 * This structure allows to do the lazy mime-type recognition, based
 * on analysis of the object name (vs content)
 */
typedef struct {
  LpgdvMimeTypeId id;
  const char*     mt_name;
  const char*     obj_ext;
  const char*     obj_regex;
} LpgdvMimeTypeDesc;

/*
 * The description table is an array of the latter structure
 */
extern LpgdvMimeTypeDesc MimeTypeDesc[];

/**
 * Determines the mime-type Id of a given object.
 *
 * @param obj_name
 *   [IN] if non-zero, the name (omitting directories) of the object
 *
 * @param fd
 *   [IN] if not equal to -1, the file descriptor of the already-
 *        opened object, which has already been lseek()-ed to its
 *        beginning
 *
 * @returns
 *   Mime-type internal identifier
 */
extern LpgdvMimeTypeId lpgdv_mime_type_id_of(const char* obj_name, int fd);


/**
 * Determines the mime-type name of a given object.
 *
 * @param id
 *   [IN] the id of the mime-type
 *
 * @returns
 *   String containing the name
 */
extern const char* lpgdv_mime_type_name_of(LpgdvMimeTypeId id);

#endif /* __LIBPG_DIRVIEW_MIME_H__ */

/*
 * The mime type description table is defined here in order to gather
 * mime-related definitions together.
 *
 * See: /usr/share/mime-info/gnome-vfs.mime
 *
 * It is meant to be declared in only one file, mime.c, by defining
 * DECLARE_MIME_TYPE_DESC
 *
 */
#ifdef DECLARE_MIME_TYPE_DESC

LpgdvMimeTypeDesc MimeTypeDesc[] = {

  /* double extension matters */

  MIME_application__x_bzip_compressed_tar,
  "application/x-bzip-compressed-tar",
  "",
  "\\.tar\\.bz2$",

  MIME_application__x_compressed_tar,
  "application/x-compressed-tar",
  "tgz",
  "\\.tar\\.gz$",

  MIME_application__x_cpio_compressed,
  "application/x-cpio-compressed",
  "",
  "\\.cpio\\.gz$",

  MIME_application__x_profile,
  "application/x-profile",
  "",
  "\\.gmon\\.out$",

  MIME_image__x_compressed_xcf,
  "image/x-compressed-xcf",
  "",
  "\\.xcf\\.gz$ \\.xcf\\.bz2$",

  /* last extension matters */

  MIME_application__andrew_inset,
  "application/andrew-inset",
  "ez",
  "",

  MIME_application__msword,
  "application/msword",
  "doc",
  "",

  MIME_application__octet_stream,
  "application/octet-stream",
  "bin",
  "",

  MIME_application__oda,
  "application/oda",
  "oda",
  "",

  MIME_application__pdf,
  "application/pdf",
  "pdf",
  "",

  MIME_application__pgp,
  "application/pgp",
  "pgp",
  "",

  MIME_application__postscript,
  "application/postscript",
  "ps eps",
  "",

  MIME_application__qif,
  "application/qif",
  "qif",
  "",

  MIME_application__rtf,
  "application/rtf",
  "rtf",
  "",

  MIME_application__vnd_lotus_1_2_3,
  "application/vnd.lotus-1-2-3",
  "123 wk1 wk3 wk4 wks",
  "",

  MIME_application__vnd_ms_excel,
  "application/vnd.ms-excel",
  "xls xla xlt xlc xld",
  "",

  MIME_application__vnd_ms_powerpoint,
  "application/vnd.ms-powerpoint",
  "ppt",
  "",

  MIME_application__vnd_stardivision_calc,
  "application/vnd.stardivision.calc",
  "sdc",
  "",

  MIME_application__vnd_stardivision_chart,
  "application/vnd.stardivision.chart",
  "sds",
  "",

  MIME_application__vnd_stardivision_draw,
  "application/vnd.stardivision.draw",
  "sda",
  "",

  MIME_application__vnd_stardivision_impress,
  "application/vnd.stardivision.impress",
  "sdd sdp",
  "",

  MIME_application__vnd_stardivision_mail,
  "application/vnd.stardivision.mail",
  "smd",
  "",

  MIME_application__vnd_stardivision_math,
  "application/vnd.stardivision.math",
  "smf",
  "",

  MIME_application__vnd_stardivision_writer,
  "application/vnd.stardivision.writer",
  "sdw vor sgl",
  "",

  MIME_application__vnd_sun_xml_calc,
  "application/vnd.sun.xml.calc",
  "sxc",
  "",

  MIME_application__vnd_sun_xml_calc_template,
  "application/vnd.sun.xml.calc.template",
  "stc",
  "",

  MIME_application__vnd_sun_xml_draw,
  "application/vnd.sun.xml.draw",
  "sxd",
  "",

  MIME_application__vnd_sun_xml_draw_template,
  "application/vnd.sun.xml.draw.template",
  "std",
  "",

  MIME_application__vnd_sun_xml_impress,
  "application/vnd.sun.xml.impress",
  "sxi",
  "",

  MIME_application__vnd_sun_xml_impress_template,
  "application/vnd.sun.xml.impress.template",
  "sti",
  "",

  MIME_application__vnd_sun_xml_math,
  "application/vnd.sun.xml.math",
  "sxm",
  "",

  MIME_application__vnd_sun_xml_writer,
  "application/vnd.sun.xml.writer",
  "sxw",
  "",

  MIME_application__vnd_sun_xml_writer_global,
  "application/vnd.sun.xml.writer.global",
  "sxg",
  "",

  MIME_application__vnd_sun_xml_writer_template,
  "application/vnd.sun.xml.writer.template",
  "stw",
  "",

  MIME_application__x_abiword,
  "application/x-abiword",
  "abw",
  "",

  MIME_application__x_applix_spreadsheet,
  "application/x-applix-spreadsheet",
  "as",
  "",

  MIME_application__x_applix_word,
  "application/x-applix-word",
  "aw",
  "",

  MIME_application__x_arj,
  "application/x-arj",
  "arj",
  "",

  MIME_application__x_asp,
  "application/x-asp",
  "asp",
  "",

  MIME_application__x_backup,
  "application/x-backup",
  "bak BAK",
  "",

  MIME_application__x_bcpio,
  "application/x-bcpio",
  "bcpio",
  "",

  MIME_application__x_bzip,
  "application/x-bzip",
  "bz2 bz",
  "",

  MIME_application__x_cgi,
  "application/x-cgi",
  "cgi",
  "",

  MIME_application__x_chess_pgn,
  "application/x-chess-pgn",
  "pgn",
  "",

  MIME_application__x_compress,
  "application/x-compress",
  "Z",
  "",

  MIME_application__x_core_file,
  "application/x-core-file",
  "",
  "^core$",

  MIME_application__x_cpio,
  "application/x-cpio",
  "cpio",
  "",

  MIME_application__x_dc_rom,
  "application/x-dc-rom",
  "dc",
  "",

  MIME_application__x_deb,
  "application/x-deb",
  "deb",
  "",

  MIME_application__x_dia_diagram,
  "application/x-dia-diagram",
  "dia",
  "",

  MIME_application__x_dvi,
  "application/x-dvi",
  "dvi",
  "",

  MIME_application__x_e_theme,
  "application/x-e-theme",
  "etheme ETHEME",
  "",

  MIME_application__x_font_afm,
  "application/x-font-afm",
  "afm",
  "",

  MIME_application__x_font_bdf,
  "application/x-font-bdf",
  "bdf",
  "",

  MIME_application__x_font_linux_psf,
  "application/x-font-linux-psf",
  "psf",
  "",

  MIME_application__x_font_pcf,
  "application/x-font-pcf",
  "pcf",
  "",

  MIME_application__x_font_speedo,
  "application/x-font-speedo",
  "spd",
  "",

  MIME_application__x_font_ttf,
  "application/x-font-ttf",
  "ttf TTF",
  "",

  MIME_application__x_font_type1,
  "application/x-font-type1",
  "pfa pfb",
  "",

  MIME_application__x_gameboy_rom,
  "application/x-gameboy-rom",
  "gb",
  "",

  MIME_application__x_genesis_rom,
  "application/x-genesis-rom",
  "gen md",
  "",

  MIME_application__x_glade,
  "application/x-glade",
  "glade",
  "",

  MIME_application__x_gnome_app_info,
  "application/x-gnome-app-info",
  "desktop",
  "",

  MIME_application__x_gnucash,
  "application/x-gnucash",
  "gnucash gnc xac",
  "",

  MIME_application__x_gnumeric,
  "application/x-gnumeric",
  "gnumeric",
  "",

  MIME_application__x_gtar,
  "application/x-gtar",
  "gtar",
  "",

  MIME_application__x_gzip,
  "application/x-gzip",
  "gz",
  "",

  MIME_application__x_hdf,
  "application/x-hdf",
  "hdf",
  "",

  MIME_application__x_java_byte_code,
  "application/x-java-byte-code",
  "class",
  "",

  MIME_application__x_jbuilder_project,
  "application/x-jbuilder-project",
  "jpr jpx",
  "",

  MIME_application__x_kde_app_info,
  "application/x-kde-app-info",
  "kdelnk",
  "",

  MIME_application__x_killustrator,
  "application/x-killustrator",
  "kil",
  "",

  MIME_application__x_kpresenter,
  "application/x-kpresenter",
  "kpr",
  "",

  MIME_application__x_kspread,
  "application/x-kspread",
  "ksp",
  "",

  MIME_application__x_kword,
  "application/x-kword",
  "kwd",
  "",

  MIME_application__x_lha,
  "application/x-lha",
  "lha",
  "",

  MIME_application__x_lhz,
  "application/x-lhz",
  "lhz",
  "",

  MIME_application__x_mif,
  "application/x-mif",
  "mif",
  "",

  MIME_application__x_mrproject,
  "application/x-mrproject",
  "mrp",
  "",

  MIME_application__x_ms_dos_executable,
  "application/x-ms-dos-executable",
  "exe",
  "",

  MIME_application__x_msx_rom,
  "application/x-msx-rom",
  "msx",
  "",

  MIME_application__x_n64_rom,
  "application/x-n64-rom",
  "n64",
  "",

  MIME_application__x_nes_rom,
  "application/x-nes-rom",
  "nes",
  "",

  MIME_application__x_netcdf,
  "application/x-netcdf",
  "cdf nc",
  "",

  MIME_application__x_object_file,
  "application/x-object-file",
  "o",
  "",

  MIME_application__x_ogg,
  "application/x-ogg",
  "ogg",
  "",

  MIME_application__x_oleo,
  "application/x-oleo",
  "oleo",
  "",

  MIME_application__x_php,
  "application/x-php",
  "php php3 php4",
  "",

  MIME_application__x_python_byte_code,
  "application/x-python-byte-code",
  "pyc",
  "",

  MIME_application__x_qw,
  "application/x-qw",
  "qif",
  "",

  MIME_application__x_rar,
  "application/x-rar",
  "rar",
  "",

  MIME_application__x_rar_compressed,
  "application/x-rar-compressed",
  "rar",
  "",

  MIME_application__x_reject,
  "application/x-reject",
  "rej",
  "",

  MIME_application__x_rpm,
  "application/x-rpm",
  "rpm",
  "",

  MIME_application__x_shar,
  "application/x-shar",
  "shar",
  "",

  MIME_application__x_shared_library,
  "application/x-shared-library",
  "so",
  "",

  MIME_application__x_shared_library_la,
  "application/x-shared-library-la",
  "la",
  "",

  MIME_application__x_shockwave_flash,
  "application/x-shockwave-flash",
  "swf",
  "",

  MIME_application__x_smil,
  "application/x-smil",
  "smil smi sml",
  "",

  MIME_application__x_sms_rom,
  "application/x-sms-rom",
  "sms gg",
  "",

  MIME_application__x_sv4cpio,
  "application/x-sv4cpio",
  "sv4cpio",
  "",

  MIME_application__x_sv4crc,
  "application/x-sv4crc",
  "sv4crc",
  "",

  MIME_application__x_tar,
  "application/x-tar",
  "tar",
  "",

  MIME_application__x_theme,
  "application/x-theme",
  "theme",
  "",

  MIME_application__x_troff_man_compressed,
  "application/x-troff-man-compressed",
  "",
  "([^0-9]|^[^\\.]*)\\.([1-9][a-z]?|n)\\.g?[Zz]$",

  MIME_application__x_unix_archive,
  "application/x-unix-archive",
  "a",
  "",

  MIME_application__x_ustar,
  "application/x-ustar",
  "ustar",
  "",

  MIME_application__x_wais_source,
  "application/x-wais-source",
  "src",
  "",

  MIME_application__x_xbase,
  "application/x-xbase",
  "dbf",
  "",

  MIME_application__x_zoo,
  "application/x-zoo",
  "zoo",
  "",

  MIME_application__zip,
  "application/zip",
  "zip",
  "",

  MIME_audio__ac3,
  "audio/ac3",
  "ac3",
  "",

  MIME_audio__basic,
  "audio/basic",
  "snd",
  "",

  MIME_audio__prs_sid,
  "audio/prs.sid",
  "sid psid",
  "",

  MIME_audio__x_aiff,
  "audio/x-aiff",
  "aif aifc aiff",
  "",

  MIME_audio__x_it,
  "audio/x-it",
  "it IT",
  "",

  MIME_audio__x_midi,
  "audio/x-midi",
  "midi mid",
  "",

  MIME_audio__x_mod,
  "audio/x-mod",
  "mod MOD",
  "",

  MIME_audio__x_mp3,
  "audio/x-mp3",
  "mp3",
  "",

  MIME_audio__x_mpegurl,
  "audio/x-mpegurl",
  "m3u",
  "",

  MIME_audio__x_real_audio,
  "audio/x-real-audio",
  "rm ra ram",
  "",

  MIME_audio__x_s3m,
  "audio/x-s3m",
  "S3M s3m",
  "",

  MIME_audio__x_stm,
  "audio/x-stm",
  "STM stm",
  "",

  MIME_audio__x_ulaw,
  "audio/x-ulaw",
  "au",
  "",

  MIME_audio__x_voc,
  "audio/x-voc",
  "voc",
  "",

  MIME_audio__x_wav,
  "audio/x-wav",
  "wav",
  "",

  MIME_audio__x_xi,
  "audio/x-xi",
  "xi",
  "",

  MIME_audio__x_xm,
  "audio/x-xm",
  "xm XM",
  "",

  MIME_image__bmp,
  "image/bmp",
  "bmp",
  "",

  MIME_image__cgm,
  "image/cgm",
  "cgm",
  "",

  MIME_image__gif,
  "image/gif",
  "gif",
  "",

  MIME_image__ief,
  "image/ief",
  "ief",
  "",

  MIME_image__jpeg,
  "image/jpeg",
  "jpe jpeg jpg",
  "",

  MIME_image__png,
  "image/png",
  "png",
  "",

  MIME_image__svg,
  "image/svg",
  "svg",
  "",

  MIME_image__tiff,
  "image/tiff",
  "tif tiff",
  "",

  MIME_image__vnd_dwg,
  "image/vnd.dwg",
  "dwg",
  "",

  MIME_image__vnd_dxf,
  "image/vnd.dxf",
  "dxf",
  "",

  MIME_image__x_3ds,
  "image/x-3ds",
  "3ds",
  "",

  MIME_image__x_applix_graphic,
  "image/x-applix-graphic",
  "ag",
  "",

  MIME_image__x_bmp,
  "image/x-bmp",
  "bmp",
  "",

  MIME_image__x_cmu_raster,
  "image/x-cmu-raster",
  "ras",
  "",

  MIME_image__x_emf,
  "image/x-emf",
  "emf",
  "",

  MIME_image__x_ico,
  "image/x-ico",
  "ico",
  "",

  MIME_image__x_iff,
  "image/x-iff",
  "iff IFF",
  "",

  MIME_image__x_ilbm,
  "image/x-ilbm",
  "ilbm ILBM",
  "",

  MIME_image__x_lwo,
  "image/x-lwo",
  "lwo lwob",
  "",

  MIME_image__x_lws,
  "image/x-lws",
  "lws",
  "",

  MIME_image__x_pcx,
  "image/x-pcx",
  "pcx",
  "",

  MIME_image__x_photo_cd,
  "image/x-photo-cd",
  "pcd",
  "",

  MIME_image__x_pict,
  "image/x-pict",
  "pct",
  "",

  MIME_image__x_portable_anymap,
  "image/x-portable-anymap",
  "pnm",
  "",

  MIME_image__x_portable_bitmap,
  "image/x-portable-bitmap",
  "pbm",
  "",

  MIME_image__x_portable_graymap,
  "image/x-portable-graymap",
  "pgm",
  "",

  MIME_image__x_portable_pixmap,
  "image/x-portable-pixmap",
  "ppm",
  "",

  MIME_image__x_psd,
  "image/x-psd",
  "psd",
  "",

  MIME_image__x_rgb,
  "image/x-rgb",
  "rgb",
  "",

  MIME_image__x_tga,
  "image/x-tga",
  "tga",
  "",

  MIME_image__x_wmf,
  "image/x-wmf",
  "wmf",
  "",

  MIME_image__x_xbitmap,
  "image/x-xbitmap",
  "xbm",
  "",

  MIME_image__x_xcf,
  "image/x-xcf",
  "xcf",
  "",

  MIME_image__x_xfig,
  "image/x-xfig",
  "fig",
  "",

  MIME_image__x_xpixmap,
  "image/x-xpixmap",
  "xpm",
  "",

  MIME_image__x_xwindowdump,
  "image/x-xwindowdump",
  "xwd",
  "",

  MIME_message__x_gnu_rmail,
  "message/x-gnu-rmail",
  "",
  "^RMAIL$",

  MIME_model__vrml,
  "model/vrml",
  "wrl",
  "",

  MIME_text__abiword,
  "text/abiword",
  "",
  "",

  MIME_text__bib,
  "text/bib",
  "bib",
  "",

  MIME_text__calendar,
  "text/calendar",
  "ics",
  "",

  MIME_text__css,
  "text/css",
  "css",
  "",

  MIME_text__html,
  "text/html",
  "html htm HTML",
  "",

  MIME_text__mathml,
  "text/mathml",
  "mml",
  "",

  MIME_text__plain,
  "text/plain",
  "asc txt TXT",
  "",

  MIME_text__richtext,
  "text/richtext",
  "rtx",
  "",

  MIME_text__sgml,
  "text/sgml",
  "sgml sgm",
  "",

  MIME_text__spreadsheet,
  "text/spreadsheet",
  "sylk slk",
  "",

  MIME_text__tab_separated_values,
  "text/tab-separated-values",
  "tsv",
  "",

  MIME_text__x_authors,
  "text/x-authors",
  "",
  "AUTHORS",

  MIME_text__x_c,
  "text/x-c",
  "c",
  "",

  MIME_text__x_cpp,
  "text/x-c++",
  "cc C cpp c++",
  "",

  MIME_text__x_c_header,
  "text/x-c-header",
  "h H h++ hp",
  "",

  MIME_text__x_comma_separated_values,
  "text/x-comma-separated-values",
  "csv",
  "",

  MIME_text__x_copying,
  "text/x-copying",
  "",
  "COPYING",

  MIME_text__x_credits,
  "text/x-credits",
  "",
  "CREDITS",

  MIME_text__x_csh,
  "text/x-csh",
  "csh",
  "",

  MIME_text__x_dcl,
  "text/x-dcl",
  "dcl",
  "",

  MIME_text__x_dsl,
  "text/x-dsl",
  "dsl",
  "",

  MIME_text__x_dtd,
  "text/x-dtd",
  "dtd",
  "",

  MIME_text__x_emacs_lisp,
  "text/x-emacs-lisp",
  "el",
  "",

  MIME_text__x_fortran,
  "text/x-fortran",
  "f",
  "",

  MIME_text__x_gtkrc,
  "text/x-gtkrc",
  "",
  "[.|]gtkrc",

  MIME_text__x_haskell,
  "text/x-haskell",
  "hs",
  "",

  MIME_text__x_idl,
  "text/x-idl",
  "idl",
  "",

  MIME_text__x_install,
  "text/x-install",
  "",
  "INSTALL",

  MIME_text__x_java,
  "text/x-java",
  "java",
  "",

  MIME_text__x_literate_haskell,
  "text/x-literate-haskell",
  "lhs",
  "",

  MIME_text__x_lyx,
  "text/x-lyx",
  "lyx",
  "",

  MIME_text__x_makefile,
  "text/x-makefile",
  "",
  "[Mm]akefile",

  MIME_text__x_perl,
  "text/x-perl",
  "pl perl",
  "",

  MIME_text__x_python,
  "text/x-python",
  "py",
  "",

  MIME_text__x_readme,
  "text/x-readme",
  "",
  "README.*",

  MIME_text__x_scheme,
  "text/x-scheme",
  "scm",
  "",

  MIME_text__x_setext,
  "text/x-setext",
  "etx",
  "",

  MIME_text__x_sh,
  "text/x-sh",
  "sh",
  "",

  MIME_text__x_sql,
  "text/x-sql",
  "sql",
  "",

  MIME_text__x_tcl,
  "text/x-tcl",
  "tcl",
  "",

  MIME_text__x_tex,
  "text/x-tex",
  "tex cls sty",
  "",

  MIME_text__x_texinfo,
  "text/x-texinfo",
  "texi texinfo",
  "",

  MIME_text__x_troff,
  "text/x-troff",
  "roff t tr",
  "",

  MIME_text__x_troff_man,
  "text/x-troff-man",
  "man",
  "(([^0-9]|^[^\\.]*)\\.([1-9][A-Za-z]*|n)|\\.man)$",

  MIME_text__x_troff_me,
  "text/x-troff-me",
  "me",
  "",

  MIME_text__x_troff_mm,
  "text/x-troff-mm",
  "mm",
  "",

  MIME_text__x_troff_ms,
  "text/x-troff-ms",
  "ms",
  "",

  MIME_text__x_vcalendar,
  "text/x-vcalendar",
  "vcs vcf",
  "",

  MIME_text__x_vcard,
  "text/x-vcard",
  "gcrd",
  "",

  MIME_text__xml,
  "text/xml",
  "xml",
  "",

  MIME_video__mpeg,
  "video/mpeg",
  "mp2 mpe mpeg mpg vob dat",
  "",

  MIME_video__quicktime,
  "video/quicktime",
  "mov qt",
  "",

  MIME_video__x_anim,
  "video/x-anim",
  "",
  "\\.anim[1-9j]$",

  MIME_video__x_flc,
  "video/x-flc",
  "flc",
  "",

  MIME_video__x_fli,
  "video/x-fli",
  "fli",
  "",

  MIME_video__x_ms_asf,
  "video/x-ms-asf",
  "asf asx",
  "",

  MIME_video__x_ms_wmv,
  "video/x-ms-wmv",
  "wmv",
  "",

  MIME_video__x_msvideo,
  "video/x-msvideo",
  "avi",
  "",

  MIME_video__x_sgi_movie,
  "video/x-sgi-movie",
  "movie",
  "",
};
#endif /* DECLARE_MIMETYPEDESC */
