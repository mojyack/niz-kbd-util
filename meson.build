project('app', 'cpp', version: '1.0.0', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++20'])
add_project_arguments('-Wfatal-errors', language: 'cpp')
add_project_arguments('-Wno-gnu-empty-struct', language: 'cpp')
add_project_arguments('-Wno-c99-extensions', language: 'cpp')

src = files(
  'src/main.cpp',
  'src/niz.cpp',
  'src/common.cpp',
  'src/keymap.cpp',
  'src/config.cpp',
  'src/firmware.cpp',
  'src/keycounts.cpp',
  'src/calib.cpp',
)

executable('niz-kbd-util', src, install : true)
