project(
  'hyprfocus',
  'cpp',
  default_options: ['buildtype=release'],
)

cpp_compiler = meson.get_compiler('cpp')
if cpp_compiler.has_argument('-std=c++23')
  add_global_arguments('-std=c++23', language: 'cpp')
elif cpp_compiler.has_argument('-std=c++2b')
  add_global_arguments('-std=c++2b', language: 'cpp')
else
  error(
    'Could not configure current C++ compiler ('
    + cpp_compiler.get_id()
    + ' '
    + cpp_compiler.version()
    + ') with required C++ standard (C++23)',
  )
endif

add_project_arguments(
  '-fPIC',
  '--no-gnu-unique',
  language: 'cpp',
)

src = [
  'src/main.cpp',
  'src/IFocusAnimation.cpp',
]

flash = get_option('flash')
shrink = get_option('shrink')

if flash.disabled() and shrink.disabled()
  error('You must enable at least one feature: flash or shrink')
endif

if not flash.disabled()
  src += 'src/Flash.cpp'
  add_project_arguments('-DFLASH', language: 'cpp')
endif

if not shrink.disabled()
  src += 'src/Shrink.cpp'
  add_project_arguments('-DSHRINK', language: 'cpp')
endif

if get_option('xwayland').disabled()
  add_project_arguments('-DNO_XWAYLAND', language: 'cpp')
endif

if get_option('dispatch').enabled()
  add_project_arguments('-DDISPATCH', language: 'cpp')
endif

shared_module(
  meson.project_name(),
  src,
  dependencies: [
    dependency('hyprland', version: '>=0.40'),
    dependency('pixman-1'),
    dependency('libdrm'),
  ],
  install: true,
)
