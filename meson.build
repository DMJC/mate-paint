project('mate-paint', 'cpp',
  version: '1.0.0',
  default_options: [
    'cpp_std=c++11',
    'datadir=share',
  ]
)

i18n = import('i18n')
qt5 = import('qt5')

qt5_dep = dependency('qt5', modules: ['Core', 'Gui', 'Widgets'])

icon_install_dir = join_paths(get_option('prefix'), get_option('datadir'), meson.project_name())

qt5_files = qt5.preprocess(
  moc_headers: 'mate-paint.hpp',
)

executable('mate-paint',
  'mate-paint.cpp',
  qt5_files,
  dependencies: qt5_dep,
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

install_subdir('help',
  install_dir: join_paths(get_option('datadir'))
)

subdir('po')
