proc pgDialog { title } {
	global pg_widget defaultparent defaltrship
	set dlg [pgCreateWidget $pg_widget(dialogbox)]
	pgSetText $dlg $title
	set defaultparent $dlg
	return $dlg
}
