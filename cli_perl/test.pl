#!/usr/bin/perl -w
use PicoGUI;
$pg = new PicoGUI::Server;

$app = $pg->Register($pg->mkString("Low-level cli_perl demo"),1,0);

$w = $pg->mkWidget(2,5,$app);   # Create a button inside $app

$pg->set($w,1<<3,2);                         # PG_WP_SIDE -> PG_S_TOP
$pg->set($w,$pg->mkString("Hello World"),7); # PG_WP_TEXT -> Hello World

$pg->Update;
$pg->Wait;

