adwaita = dependency('libadwaita-1')
peel = dependency('peel')

peel_gen = find_program('peel-gen')
peel_codegen = custom_target('peel-codegen',
  command: [
    peel_gen,
    '--recursive',
    '--out-dir', '@OUTDIR@',
    'Adw', '1',
  ],
  output: 'peel',
  env: {
    'GI_GIR_PATH': '/usr/lib/x86_64-linux-gnu/gir-1.0'
  },
)

executable('gig',
  'main.cpp',
  peel_codegen,
  dependencies: [adwaita, peel],
)
