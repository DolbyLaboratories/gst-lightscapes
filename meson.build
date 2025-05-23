project('gst-lightscapes', 'c',
         version : '0.1.0',
   meson_version : '>= 0.60.0',
 default_options : ['warning_level=1', 'buildtype=debugoptimized', 'c_std=c11'])

system = host_machine.system()
version = meson.project_version()

glib_req = '>= 2.40.0'
gst_req = '>= 1.16.0'

cc = meson.get_compiler('c')
host_system = host_machine.system()

py_mod = import('python')
py = py_mod.find_installation()

# config.h
core_conf = configuration_data()
core_conf.set_quoted('VERSION', meson.project_version())
core_conf.set_quoted('PACKAGE', meson.project_name())
core_conf.set_quoted('LICENSE', 'LGPL')
core_conf.set_quoted('ORIGIN', 'http://dolby.com/')

configinc = include_directories('.')

gst_plugins_dlb_args = ['-DHAVE_CONFIG_H']
gst_plugins_link_args = []

if (get_option('buildtype') == 'release')
  gst_plugins_dlb_args += '-DG_DISABLE_ASSERT'
endif

warning_flags = [
  '-Wmissing-declarations',
  '-Wredundant-decls',
  '-Wwrite-strings',
  '-Winit-self',
  '-Wmissing-include-dirs',
  '-Wno-multichar',
  '-Wvla',
  '-Wpointer-arith',
  '-Wmissing-prototypes',
  '-Wold-style-definition',
  '-Waggregate-return',
]

foreach extra_arg : warning_flags
  if cc.has_argument (extra_arg)
    add_project_arguments([extra_arg], language: 'c')
  endif
endforeach

if get_option('b_sanitize') == 'address'
  if cc.has_argument('-fno-omit-frame-pointer')
    add_project_arguments('-fno-omit-frame-pointer', language: 'c')
  endif
endif

dl_dep = cc.find_library('dl', required : false)

if dl_dep.found()
  core_conf.set('HAVE_DLADDR', 1)
elif host_system == 'windows'
  core_conf.set('HAVE_WINAPI', 1)
endif

# GLib, gobject
glib_deps = [dependency('glib-2.0', version : glib_req, fallback: ['glib', 'libglib_dep']),
             dependency('gobject-2.0', fallback: ['glib', 'libgobject_dep']),
             dependency('json-glib-1.0', fallback: ['json-glib', 'json_glib_dep'])]

# GStreamer dependencies
gst_dep = dependency('gstreamer-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_dep'])
gst_base_dep = dependency('gstreamer-base-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_base_dep'])

pkgconfig = import('pkgconfig')
plugins_install_dir = '@0@/gstreamer-1.0'.format(get_option('libdir'))
plugins_pkgconfig_install_dir = join_paths(plugins_install_dir, 'pkgconfig')

# set variable that points to this projects build directory for tests
gst_plugins_dlb_build_dir = meson.current_build_dir()

# subprojects
dep_map = {
  'dlb_lightscapes': {'option': get_option('lsm').disabled(), 'ver': '1.0.0'},
}

foreach name, info : dep_map
  ver = info.get('ver', '')
  dep = []

  if not info.get('option', false)
    dep = dependency(name, version: ver, required : false, fallback : [name, name + '_dep'])

    if dep.type_name() != 'internal'
      if dep.type_name() == 'pkgconfig'
        libdir = dep.get_variable(pkgconfig: 'libdir') + '/'
        libname = dep.get_variable(pkgconfig: 'libname')
      else
        libname = name
        libdir = ''
      endif

      core_conf.set(name.to_upper() + '_OPEN_DYNLIB', 1)
      core_conf.set_quoted(name.to_upper() + '_LIBNAME',
          '@0@lib@1@.so'.format(libdir, libname))

      lib = static_library(name + '_shim', 'shim'/name + '.c',
          c_args : ['-DHAVE_CONFIG_H'], dependencies : dl_dep)
      dep = declare_dependency(
          link_with : lib, include_directories : include_directories('shim'))
    endif
  endif
  dep_map += {name: dep}
endforeach

dlb_lightscapes_dep = dep_map.get('dlb_lightscapes')

# Plugins
plugins = []

subdir('plugins')

# Use core_conf after all subdirs have set values
configure_file(input: 'config.h.in', output : 'config.h', configuration : core_conf)

plugin_names = []
foreach plugin: plugins
  plugin_names += [plugin.name().substring(3)]
endforeach

summary({
    'Plugins': plugin_names,
}, list_sep: ', ')
