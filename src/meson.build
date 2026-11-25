gig_sources = [
  'gig-address-bar.c',
  'gig-application.c',
  'gig-find-bar.c',
  'gig-find-entry.c',
  'gig-floating-bar.c',
  'gig-open-disposition.c',
  'gig-settings.c',
  'gig-tab.c',
  'gig-uri-utils.c',
  'gig-web-view.c',
  'gig-window.c',
  'history/gig-history-dialog.c',
  'history/gig-history-service.c',
  'history/gig-history-visit.c',
  'main.c',
]

blueprints = custom_target(
  'blueprints',
  input: files(
    'history/gig-history-dialog.blp',
    'gig-address-bar.blp',
    'gig-find-bar.blp',
    'gig-find-entry.blp',
    'gig-floating-bar.blp',
    'gig-tab.blp',
    'gig-window.blp',
  ),
  output: '.',
  command: [
    find_program('blueprint-compiler'),
    'batch-compile',
    '--minify', '@OUTPUT@',
    '@CURRENT_SOURCE_DIR@',
    '@INPUT@',
  ],
)

gig_sources += gnome.compile_resources(
  'gig-resources',
  'com.github.obyknovenius.Gig.gresource.xml',
  dependencies: blueprints,
)

gtk_dep = dependency('gtk4', version: '>= 4.24')
adwaita_dep = dependency('libadwaita-1', version: '>= 1.10')
soup_dep = dependency('libsoup-3.0')
sqlite_dep = dependency('sqlite3')
webkitgtk_dep = dependency('webkitgtk-6.0')

webkit_revision = webkitgtk_dep.get_variable(pkgconfig: 'revision', default_value: '')
if webkit_revision not in ['', 'tarball']
  webkit_revision = ' (@0@)'.format(webkit_revision)
else
  webkit_revision = ''
endif

config.set_quoted('WEBKIT_REVISION', webkit_revision)

gig_deps = [
  gtk_dep,
  adwaita_dep,
  soup_dep,
  sqlite_dep,
  webkitgtk_dep,
]

executable(
  'gig',
  gig_sources,
  include_directories: include_directories('..'),
  dependencies: gig_deps,
  install: true,
)
