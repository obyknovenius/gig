project('gig', 'c')

gnome = import('gnome')

app_id = 'com.github.obyknovenius.Gig'

config = configuration_data()
config.set_quoted('APP_ID', app_id)

subdir('data')
subdir('src')

configure_file(output: 'config.h', configuration: config)

gnome.post_install(
    glib_compile_schemas: true,
    update_desktop_database: true,
)
