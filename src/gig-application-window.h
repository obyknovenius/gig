#pragma once

#include <adwaita.h>
#include <webkit/webkit.h>

G_BEGIN_DECLS

#define GIG_TYPE_APPLICATION_WINDOW (gig_application_window_get_type ())

G_DECLARE_FINAL_TYPE (GigApplicationWindow, gig_application_window, GIG, APPLICATION_WINDOW, AdwApplicationWindow)

GtkWidget *gig_application_window_new (GtkApplication *app);

WebKitWebView *gig_application_window_get_web_view (GigApplicationWindow *self);

G_END_DECLS
