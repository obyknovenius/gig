#include "gig-application-window.h"

struct _GigApplicationWindow
{
  AdwApplicationWindow parent_instance;

  WebKitWebView *web_view;
};

G_DEFINE_TYPE (GigApplicationWindow, gig_application_window, ADW_TYPE_APPLICATION_WINDOW)

static void
gig_application_window_class_init (GigApplicationWindowClass *klass)
{
}

static void
gig_application_window_init (GigApplicationWindow *self)
{
  gtk_window_set_title (GTK_WINDOW (self), "Gig");
  gtk_window_set_default_size (GTK_WINDOW (self), 1024, 768);

  self->web_view = WEBKIT_WEB_VIEW (webkit_web_view_new ());
  adw_application_window_set_content (ADW_APPLICATION_WINDOW (self), GTK_WIDGET (self->web_view));

  webkit_web_view_load_uri (self->web_view, "https://www.google.com");
}

GtkWidget *
gig_application_window_new (GtkApplication *app)
{
  return g_object_new (GIG_TYPE_APPLICATION_WINDOW,
                       "application", app,
                       NULL);
}
