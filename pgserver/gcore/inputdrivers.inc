/* Input
 * Usually this will be autoloaded by the video driver.  If no driver is
 * specified, no input. Order does not matter
 *
 * (see driverinfo.c for more info)
 */

#ifdef DRIVER_SDLINPUT
  DRV("sdlinput",&sdlinput_regfunc)
#endif
   
#ifdef DRIVER_SVGAINPUT
  DRV("svgainput",&svgainput_regfunc)
#endif

#ifdef DRIVER_NCURSESINPUT
  DRV("ncursesinput",&ncursesinput_regfunc)
#endif

#ifdef DRIVER_CHIPSLICETS
  DRV("chipslicets",&chipslicets_regfunc)
#endif

#ifdef DRIVER_EVENTBROKER
  DRV("eventbroker",&eventbroker_regfunc)
#endif

#ifdef DRIVER_R3912TS
  DRV("r3912ts",&r3912ts_regfunc)
#endif

#ifdef DRIVER_VR3TS
  DRV("vr3ts",&vr3ts_regfunc)
#endif

#ifdef DRIVER_TUXTS
  DRV("tuxts",&tuxts_regfunc)
#endif

#ifdef DRIVER_UCB1X00
  DRV("ucb1x00",&ucb1x00_regfunc)
#endif

#ifdef DRIVER_TTYKB
  DRV("ttykb",&ttykb_regfunc)
#endif

#ifdef DRIVER_REMORAKB
  DRV("remorakb",&remorakb_regfunc)
#endif

#ifdef DRIVER_SERIALMOUSE
  DRV("serialmouse",&serialmouse_regfunc)
#endif

#ifdef DRIVER_X11INPUT
  DRV("x11input",&x11input_regfunc)
#endif

#ifdef DRIVER_PALMAXTS
  DRV("palmaxts",&palmaxts_regfunc)
#endif

#ifdef DRIVER_GPM
  DRV("gpm",&gpm_regfunc)
#endif

#ifdef DRIVER_MGL2INPUT
  DRV("mgl2input", &mgl2input_regfunc)
#endif

/* The End */
