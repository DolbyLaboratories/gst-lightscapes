plugin_opts = ['lsm', 'lsm_sink']

foreach plugin : plugin_opts
  if not get_option(plugin).disabled()
    subdir(plugin)
  endif
endforeach
