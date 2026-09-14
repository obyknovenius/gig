desktop_file_config = configuration_data()
desktop_file_config.set('app_id', app_id)
desktop_file = configure_file(
    input: 'com.github.obyknovenius.Gig.desktop.in',
    output: '@0@.desktop'.format(app_id),
    configuration: desktop_file_config,
    install: true,
    install_dir: join_paths(get_option('datadir'), 'applications'),
)

desktop_utils = find_program('desktop-file-validate', required: false)
if desktop_utils.found()
    test('Validate desktop file', desktop_utils, args: [desktop_file])
endif

install_data(
    desktop_file,
    install_dir: join_paths(get_option('datadir'), 'applications'),
)

gnome.compile_schemas(build_by_default: true)
install_data(
    'com.github.obyknovenius.Gig.gschema.xml',
    install_dir: join_paths(get_option('datadir'), 'glib-2.0/schemas'),
)

subdir('icons')
