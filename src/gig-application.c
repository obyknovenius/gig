#include "gig-application.h"

#include "gig-window.h"

struct _GigApplication
{
  AdwApplication parent_instance;
};

G_DEFINE_TYPE (GigApplication, gig_application, ADW_TYPE_APPLICATION)

static void
gig_application_constructed (GObject *object)
{
  GigApplication *self = GIG_APPLICATION (object);

  g_assert (GIG_IS_APPLICATION (self));

  g_application_set_application_id (G_APPLICATION (self), "com.github.obyknovenius.Gig");
  g_application_set_flags (G_APPLICATION (self), G_APPLICATION_HANDLES_OPEN);
  g_application_set_option_context_parameter_string (G_APPLICATION (self), "[FILES…]");

  G_OBJECT_CLASS (gig_application_parent_class)->constructed (object);
}

static void
gig_application_activate (GApplication *application)
{
  GigWindow *window;

  g_assert (GIG_IS_APPLICATION (application));

  window = gig_window_new (GTK_APPLICATION (application));
  gtk_window_present (GTK_WINDOW (window));
}

static void
gig_application_open (GApplication *application,
                      GFile **files,
                      gint n_files,
                      const gchar *hint)
{
  GigWindow *window;

  g_assert (GIG_IS_APPLICATION (application));

  for (gint i = 0; i < n_files; i++)
    {
      g_autofree gchar *uri = g_file_get_uri (files[i]);
      window = gig_window_new (GTK_APPLICATION (application));
      gig_window_add_tab (GIG_WINDOW (window), uri);
      gtk_window_present (GTK_WINDOW (window));
    }
}

static void
gig_application_class_init (GigApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

  object_class->constructed = gig_application_constructed;

  application_class->activate = gig_application_activate;
  application_class->open = gig_application_open;
}

static void
gig_application_init (GigApplication *self)
{
}

GigApplication *
gig_application_new (void)
{
  return g_object_new (GIG_TYPE_APPLICATION, NULL);
}
