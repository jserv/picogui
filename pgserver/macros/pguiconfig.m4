dnl AM_PGUI_CONFIG(CFG, CONFIG_1 CONFIG_2..., <action if one is true>[, <action if none is true>])

AC_DEFUN(AM_PGUI_CONFIG, [

for config in [$2] ; do
  _pgui_config_done="n"

  if . [$1] && eval "test \"\${$config}\" = \"y\"" ; then
     _pgui_config_done="y"

    ifelse([$3], , :, [$3])

     break
  fi
done

if test ${_pgui_config_done} = "n" ; then
  ifelse([$4], , :, $4)
fi
])
