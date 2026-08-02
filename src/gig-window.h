#pragma once

#include <adwaita.h>
#include <webkit/webkit.h>

G_BEGIN_DECLS

#define GIG_TYPE_WINDOW (gig_window_get_type ())

G_DECLARE_FINAL_TYPE (GigWindow, gig_window, GIG, WINDOW, AdwApplicationWindow)

GigWindow *gig_window_new (GtkApplication *app);

WebKitWebView *gig_window_get_web_view (GigWindow *self);

G_END_DECLS
