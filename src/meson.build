gig_sources = [
  'gig-application.c',
  'gig-window.c',
  'main.c',
]

blueprints = custom_target(
  'blueprints',
  input: files(
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

adwaita_dep = dependency('libadwaita-1')
webkit_dep = dependency('webkitgtk-6.0')

gig_deps = [
  adwaita_dep,
  webkit_dep,
]

executable(
  'gig',
  gig_sources,
  dependencies: gig_deps,
)
