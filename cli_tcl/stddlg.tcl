#!/usr/bin/tclsh

proc pgDialog { title } {
	global pg_wp pg_th_o
	set dlg [pgNewPopup 0 0]
	set lbl [pgNewLabel "Boing!!!"]
	pgSetWidget $lbl $pg_wp(transparent) 0
	pgSetWidget $lbl $pg_wp(thobj) $pg_th_o(label_dlgtitle)
	return $dlg
}
