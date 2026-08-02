#include "gig-application.h"

#include "gig-window.h"

struct _GigApplication
{
  AdwApplication parent_instance;
};

G_DEFINE_TYPE (GigApplication, gig_application, ADW_TYPE_APPLICATION)

static void
gig_application_activate (GApplication *application)
{
  GigWindow *window;
  WebKitWebView *web_view;

  g_assert (GIG_IS_APPLICATION (application));

  window = gig_window_new (GTK_APPLICATION (application));

  web_view = gig_window_get_web_view (GIG_WINDOW (window));
  webkit_web_view_load_uri (web_view, "https://www.google.com");

  gtk_window_present (GTK_WINDOW (window));
}

static void
gig_application_class_init (GigApplicationClass *klass)
{
  GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

  application_class->activate = gig_application_activate;
}

static void
gig_application_init (GigApplication *self)
{
}

GigApplication *
gig_application_new (void)
{
  return g_object_new (GIG_TYPE_APPLICATION,
                       "application-id", "com.github.obyknovenius.Gig",
                       "flags", G_APPLICATION_DEFAULT_FLAGS,
                       NULL);
}
