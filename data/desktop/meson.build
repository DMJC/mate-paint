i18n = import('i18n')

i18n.merge_file(
  type: 'desktop',
  input: 'mate-paint.desktop.in.in',
  output: 'mate-paint.desktop',
  po_dir: join_paths(meson.project_source_root(), 'po'),
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications'),
)
