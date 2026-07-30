adwaita = dependency('libadwaita-1')
peel = dependency('peel')
webkit = dependency('webkitgtk-6.0')

gig_deps = [
  adwaita,
  peel,
  webkit,
]

peel_gen = find_program('peel-gen')
peel_codegen = custom_target('peel-codegen',
  command: [
    peel_gen,
    '--recursive',
    '--out-dir', '@OUTDIR@',
    'Adw', '1',
    'WebKit', '6.0',
  ],
  output: 'peel',
  env: {
    'GI_GIR_PATH': '/usr/lib/x86_64-linux-gnu/gir-1.0'
  },
)

gig_sources = [
  'GigApplication.cpp',
  'GigApplicationWindow.cpp',
  'main.cpp',
]

executable('gig',
  gig_sources,
  peel_codegen,
  dependencies: gig_deps,
)
