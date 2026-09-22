project(
	'reinstall', 
	['cpp'],
	version: '7.0.0',
	default_options : ['cpp_std=c++17', 'buildtype=release' ],
	license: 'GPL-3.0-or-later',
)

app_src = [
	'src/application/main.cc',
	'src/application/application.cc',
	'src/ui/console/application.cc',
]

lib_src = [
	'src/library/testcases.cc',
	'src/library/group.cc',
]

project_description = 'Installation media creation tool'

pkg_version = meson.project_version()
version_array = pkg_version.split('.')
pkg_major_version = version_array[0].to_int()
pkg_minor_version = version_array[1].to_int()
pkg_micro_version = version_array[2].to_int()

libversion = '@0@.@1@'.format(pkg_major_version,pkg_minor_version)

cxx = meson.get_compiler('cpp')

subdir('po')

#
# Compiler flags
#
enable_debug = get_option('debug') or get_option('buildtype').contains('debug')

time_epoch = run_command('sh', '-c', 'echo "$SOURCE_DATE_EPOCH"', check : true).stdout().strip()
if time_epoch == ''
  time_epoch = run_command('date','+%Y%m%d', check: true).stdout().strip()
endif

compiler_flags_common = [
	'-DBUILD_DATE='+time_epoch,
	'-Wno-overloaded-virtual',
]

if enable_debug
  compiler_flags_common += [
    '-ggdb3',
	'-DDEBUG=1'
  ]
else
  compiler_flags_common += [
	'-DNDEBUG=1'
  ]
endif

global_cxxflags = cxx.get_supported_arguments(compiler_flags_common)

add_project_arguments(global_cxxflags, language: 'cpp')

#
# Dependencies
#
libslp = cxx.find_library('slp', required: false)
gtkmm = dependency('gtkmm-4.0', required: false)
libisofs =	dependency('libisofs-1',required: false)
libarchive = dependency('libarchive', required: false)
zlib= dependency('zlib', required: false)

fatfs =	dependency(
	'libfatfs',
	static: true,
	fallback: [ 'libfatfs' ],
	default_options: ['default_library=static'],
)

libudjat = dependency(
	'libudjat',
	version : '>=3.0.0',
	fallback: [ 'libudjat' ],
	default_options: ['default_library=static'],
)

deps = [
	libudjat,
	gtkmm,
	libisofs,
	libslp,
	zlib,
	fatfs,
	libarchive,
	cxx.find_library('dl', required: false),
	dependency(
		'libudjathttp',
		version : '>=3.0.0',
		fallback: [ 'libudjathttp' ],
	    default_options: ['default_library=static'],
	),
	dependency(
		'libudjatdbus',
		version : '>=3.0.0',
		fallback: [ 'libudjatdbus' ],
	    default_options: ['default_library=static'],
	),
]

if gtkmm.found()
	deps += dependency(
		'pwwidgets',
		version : '>=1.0.0',
		fallback: [ 'pwwidgets' ],
	    default_options: ['default_library=static'],
	)
endif

#
# Configuration
#
conf_data = configuration_data()
conf_data.set_quoted('PACKAGE_NAME', meson.project_name())
conf_data.set_quoted('PACKAGE_VERSION', meson.project_version())
conf_data.set_quoted('PACKAGE_DESCRIPTION', project_description)
conf_data.set_quoted('PACKAGE_STRING','@0@ @1@'.format(meson.project_name(), meson.project_version()))
conf_data.set('PACKAGE_DOMAIN','@0@.@1@'.format(libudjat.get_variable('product_domain'),meson.project_name()))
conf_data.set_quoted('PRODUCT_ID', meson.project_name())
conf_data.set_quoted('MODULE_VERSION',libversion)
conf_data.set_quoted('DATADIR',get_option('prefix') + '/' + get_option('datadir'))
conf_data.set_quoted('LOCALEDIR',get_option('prefix') + '/' + get_option('localedir'))

conf_data.set('application_description',project_description)
conf_data.set('application_icon','@0@.@1@'.format(libudjat.get_variable('product_domain'),meson.project_name()))
conf_data.set('application_name', meson.project_name())

conf_data.set('STATIC_MODULES',1)

if fatfs.found()
	conf_data.set('HAVE_FATFS', 1)
endif

if(zlib.found())
	conf_data.set('HAVE_ZLIB', 1)
endif

if libisofs.found()
	conf_data.set('HAVE_LIBISOFS', 1)
	conf_data.set('HAVE_ISOFS', 1)
	conf_data.set('HAVE_ISOHYBRID', 1)
	conf_data.set('HAVE_ISOIMAGEDIR', 1)
endif

if libarchive.found()
	conf_data.set('HAVE_ARCHIVE', 1)
endif

if gtkmm.found()
	conf_data.set('HAVE_GTKMM', 1)
	app_src += [
	]
endif

if libslp.found()
  conf_data.set('HAVE_LIBSLP', 1)
endif

if cxx.compiles('#include <unistd.h>', name : 'unistd.h')
  conf_data.set('HAVE_UNISTD_H', 1)
endif

configure_file(output : 
	'config.h', 
	configuration : conf_data
)

includes_dir = include_directories('../src/include')

executable(
	meson.project_name(),
	app_src + lib_src,
	install: true,
	dependencies: deps,
	include_directories: includes_dir
)

custom_target(
	input: 'icons/logo.svg',
	output: '@0@.@1@.svg'.format(libudjat.get_variable('product_domain'),meson.project_name()),
	install: true,
	install_dir: '/usr/share/icons/hicolor/scalable/apps',
	command: [ 'scour', '@INPUT@', '@OUTPUT@']
)

custom_target(
	input: 'icons/logo.svg',
	output: 'logo.svg',
	install: true,
	install_dir:'/usr/share/@0@/icons'.format(meson.project_name()),
	command: [ 'scour', '@INPUT@', '@OUTPUT@']
)

custom_target(
	input: 'icons/opensuse-leap.svg',
	output: 'opensuse-leap.svg',
	install: true,
	install_dir:'/usr/share/@0@/icons'.format(meson.project_name()),
	command: [ 'scour', '@INPUT@', '@OUTPUT@']
)

custom_target(
	input: 'icons/opensuse-tumbleweed.svg',
	output: 'opensuse-tumbleweed.svg',
	install: true,
	install_dir:'/usr/share/@0@/icons'.format(meson.project_name()),
	command: [ 'scour', '@INPUT@', '@OUTPUT@']
)

install_data(
	'stylesheet.css',
	install_dir:'/usr/share/@0@'.format(meson.project_name())
)

install_data(
configure_file(
        input: 'sysconf/launcher.desktop.in',
        output: '@0@.@1@.desktop'.format(libudjat.get_variable('product_domain'),meson.project_name()), 
        configuration: conf_data,
	),
	install_dir:'/usr/share/applications' 
)

install_data(
configure_file(
        input: 'sysconf/tmpfiles.d.in',
        output: 'tmpfiles', 
        configuration: conf_data,
	),
	rename: '@0@.conf'.format(meson.project_name()),
	install_dir:'/etc/tmpfiles.d' 
)

install_data(
	[
		'templates/grub.cfg',
		'templates/grub2',
		'templates/isolinux.cfg',
	],
	install_dir:'/usr/share/@0@/templates'.format(meson.project_name())
)

install_data(
	[
		'models/mini.xml',
		'models/net.xml',
		'models/full.xml',
		'models/grub.xml',
	],
	install_dir:'/usr/share/@0@/models'.format(meson.project_name())
)
