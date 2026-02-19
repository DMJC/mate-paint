project('mate-paint', 'cpp',
  version: '1.0.0',
  default_options: [
    'cpp_std=c++11',
    'datadir=share',
  ]
)

i18n = import('i18n')

gtk_dep = dependency('gtk+-3.0')

icon_install_dir = join_paths(get_option('prefix'), get_option('datadir'), meson.project_name())

executable('mate-paint',
  'mate-paint.cpp',
  dependencies: gtk_dep,
  cpp_args: [
    '-DICON_INSTALL_DIR="' + icon_install_dir + '"',
    '-DGETTEXT_PACKAGE="mate-paint"',
    '-DLOCALEDIR="' + join_paths(get_option('prefix'), get_option('localedir')) + '"',
  ],
  install: true
)

install_subdir('data', install_dir: icon_install_dir)

install_data('mate-paint.desktop',
  install_dir: join_paths(get_option('prefix'), get_option('datadir'), 'applications')
)

subdir('po')
