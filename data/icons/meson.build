icondir = get_option('datadir') / 'icons' / 'hicolor'

scalabledir = icondir / 'scalable' / 'apps'
install_data('@0@.svg'.format(app_id), install_dir: scalabledir)

symbolicdir = icondir / 'symbolic' / 'apps'
install_data('@0@-symbolic.svg'.format(app_id), install_dir: symbolicdir)
