dlb_light_base_sink_sources = [
  'dlblightbasesink.c'
]

dlblight = library('gstlight', dlb_light_base_sink_sources,
               c_args : gst_plugins_dlb_args,
            link_args : gst_plugins_link_args,
  include_directories : [configinc],
         dependencies : glib_deps + gst_base_dep,
              install : true,
          install_dir : plugins_install_dir
)

light_dep = declare_dependency(link_with : dlblight, include_directories: include_directories('.'))

dlb_lighttextsink_sources = [
  'dlblighttextsink.c',
]

dlblighttextsink = library('gstdlblighttextsink', dlb_lighttextsink_sources,
               c_args : gst_plugins_dlb_args,
            link_args : gst_plugins_link_args,
  include_directories : [configinc],
         dependencies : glib_deps + [gst_base_dep, dlb_lightscapes_dep, light_dep],
              install : true,
          install_dir : plugins_install_dir
)

plugins += [dlblighttextsink]
