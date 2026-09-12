desktop_file = 'com.github.obyknovenius.Gig.desktop'

desktop_utils = find_program('desktop-file-validate', required: false)
if desktop_utils.found()
    test('Validate desktop file', desktop_utils, args: [desktop_file])
endif

install_data(
    desktop_file,
    install_dir: join_paths(get_option('datadir'), 'applications'),
)

gnome.compile_schemas(build_by_default: true)
install_data('com.github.obyknovenius.Gig.gschema.xml',
  install_dir: join_paths(get_option('datadir'), 'glib-2.0/schemas')
)
