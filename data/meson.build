desktop_file_config = configuration_data()
desktop_file_config.set('app_id', app_id)
desktop_file = configure_file(
    input: 'com.github.obyknovenius.Gig.desktop.in',
    output: '@0@.desktop'.format(app_id),
    configuration: desktop_file_config,
    install: true,
    install_dir: get_option('datadir') / 'applications',
)
desktop_utils = find_program('desktop-file-validate', required: false, disabler: true)
test(
    'Validate desktop file',
    desktop_utils,
    args: [desktop_file],
)

schema_config = configuration_data()
schema_config.set('APP_SCHEMA_ID', app_schema_id)
schema_config.set('APP_SCHEMA_PATH', app_schema_path)
configure_file(
    input: 'com.github.obyknovenius.Gig.gschema.xml.in',
    output: '@0@.gschema.xml'.format(app_schema_id),
    configuration: schema_config,
    install_dir: get_option('datadir') / 'glib-2.0' / 'schemas',
)
compile_schemas = find_program('glib-compile-schemas', required: false, disabler: true)
test(
    'Validate schema file',
    compile_schemas,
    args: ['--strict', '--dry-run', meson.current_build_dir()],
)

subdir('icons')
