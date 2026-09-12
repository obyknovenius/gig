desktop_file = 'com.github.obyknovenius.Gig.desktop'

desktop_utils = find_program('desktop-file-validate', required: false)
if desktop_utils.found()
    test('Validate desktop file', desktop_utils, args: [desktop_file])
endif

install_data(
    desktop_file,
    install_dir: join_paths(get_option('datadir'), 'applications'),
)
