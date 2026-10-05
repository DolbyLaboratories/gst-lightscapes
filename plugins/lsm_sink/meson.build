dlb_light_base_sink_sources = [
  'dlblightbasesink.c'
]

# gstlight is an internal helper (the base sink) that the sink plugins link
# against; it is not itself a loadable GStreamer plugin. On Windows MSVC does
# not export DLL symbols by default, so a shared gstlight would produce no
# import .lib and the sink plugins would fail to link (LNK1181). Build it
# statically there; keep it shared on other platforms (unchanged behaviour).
gstlight_kwargs = {
  'c_args' : gst_plugins_dlb_args,
  'link_args' : gst_plugins_link_args,
  'include_directories' : [configinc],
  'dependencies' : glib_deps + gst_base_dep,
}
if host_machine.system() == 'windows'
  dlblight = static_library('gstlight', dlb_light_base_sink_sources,
    kwargs : gstlight_kwargs,
  )
else
  dlblight = library('gstlight', dlb_light_base_sink_sources,
    kwargs : gstlight_kwargs,
    install : true,
    install_dir : plugins_install_dir,
  )
endif

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

fs = import('fs')
# This subdir is optional: it is absent in some source distributions.
if fs.is_dir('internal') and not get_option('light_drivers').disabled()
    subdir('internal')
endif
