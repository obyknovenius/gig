#include "gig-window.h"

struct _GigWindow
{
  AdwApplicationWindow parent_instance;

  WebKitWebView *web_view;
};

G_DEFINE_TYPE (GigWindow, gig_window, ADW_TYPE_APPLICATION_WINDOW)

static void
gig_window_class_init (GigWindowClass *klass)
{
}

static void
gig_window_init (GigWindow *self)
{
  gtk_window_set_title (GTK_WINDOW (self), "Gig");
  gtk_window_set_default_size (GTK_WINDOW (self), 1024, 768);

  self->web_view = WEBKIT_WEB_VIEW (webkit_web_view_new ());
  adw_application_window_set_content (ADW_APPLICATION_WINDOW (self), GTK_WIDGET (self->web_view));
}

GigWindow *
gig_window_new (GtkApplication *app)
{
  return g_object_new (GIG_TYPE_WINDOW,
                       "application", app,
                       NULL);
}

WebKitWebView *
gig_window_get_web_view (GigWindow *self)
{
  g_return_val_if_fail (GIG_IS_WINDOW (self), NULL);

  return self->web_view;
}
