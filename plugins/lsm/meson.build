dlblsmparse_sources = [
  'dlblsmparse.c',
  'dlblsmparse.h',
]

dlblsmparse = library('gstdlblsmparse', dlblsmparse_sources,
               c_args : gst_plugins_dlb_args,
            link_args : gst_plugins_link_args,
  include_directories : configinc,
         dependencies : glib_deps + gst_base_dep,
              install : true,
          install_dir : plugins_install_dir,
)

dlb_lightning_sources = [
  'dlblightning.c',
]
libm = cc.find_library('m', required : false)

dlblightning = library('gstdlblightning', dlb_lightning_sources,
               c_args : gst_plugins_dlb_args,
            link_args : gst_plugins_link_args,
  include_directories : configinc,
         dependencies : glib_deps + gst_base_dep + dlb_lightscapes_dep + libm,
              install : true,
          install_dir : plugins_install_dir
)

plugins += [dlblsmparse, dlblightning]
