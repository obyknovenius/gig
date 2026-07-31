adwaita = dependency('libadwaita-1')
webkit = dependency('webkitgtk-6.0')

gig_deps = [
  adwaita,
  webkit,
]

gig_sources = [
  'gig-application.c',
  'gig-application-window.c',
  'main.c',
]

executable(
  'gig',
  gig_sources,
  dependencies: gig_deps,
)
