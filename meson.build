project('gig', 'c', version: '0.1.0')

gnome = import('gnome')

if get_option('development')
    app_id = 'com.github.obyknovenius.Gig.Devel'
else
    app_id = 'com.github.obyknovenius.Gig'
endif

config = configuration_data()
config.set_quoted('APP_ID', app_id)
config.set_quoted('PACKAGE_VERSION', meson.project_version())
config.set10('DEVELOPMENT_BUILD', get_option('development'))

subdir('data')
subdir('src')

configure_file(output: 'config.h', configuration: config)

gnome.post_install(
    glib_compile_schemas: true,
    update_desktop_database: true,
)
