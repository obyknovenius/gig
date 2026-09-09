#include "config.h"

#include "gig-application.h"

#include "gig-page.h"
#include "gig-web-view.h"
#include "gig-window.h"

struct _GigApplication
{
  AdwApplication parent_instance;
};

G_DEFINE_TYPE (GigApplication, gig_application, ADW_TYPE_APPLICATION)

static GigWindow *
find_or_create_window (GigApplication *self)
{
  const GList *windows = NULL;

  g_assert (GIG_IS_APPLICATION (self));

  windows = gtk_application_get_windows (GTK_APPLICATION (self));

  /* Try to find the most recent editor window displayed */
  for (const GList *iter = windows; iter; iter = iter->next)
    {
      GtkWindow *window = iter->data;

      g_assert (GTK_IS_WINDOW (window));

      if (GIG_IS_WINDOW (window))
        return GIG_WINDOW (window);
    }

  return gig_window_new (GTK_APPLICATION (self));
}

static void
gig_application_constructed (GObject *object)
{
  GigApplication *self = GIG_APPLICATION (object);
  WebKitNetworkSession *network_session;
  WebKitWebsiteDataManager *data_manager;
  WebKitCookieManager *cookie_manager;
  g_autofree gchar *data_dir;
  g_autofree gchar *cookies_path;

  g_assert (GIG_IS_APPLICATION (self));

  g_application_set_application_id (G_APPLICATION (self), APP_ID);
  g_application_set_flags (G_APPLICATION (self), G_APPLICATION_HANDLES_OPEN);
  g_application_set_option_context_parameter_string (G_APPLICATION (self), "[FILES…]");

  network_session = webkit_network_session_get_default ();

  data_manager = webkit_network_session_get_website_data_manager (network_session);
  webkit_website_data_manager_set_favicons_enabled (data_manager, TRUE);

  data_dir = g_build_filename (g_get_user_data_dir (), APP_ID, NULL);
  g_mkdir_with_parents (data_dir, 0700);

  cookies_path = g_build_filename (data_dir, "cookies.sqlite", NULL);

  cookie_manager = webkit_network_session_get_cookie_manager (network_session);
  webkit_cookie_manager_set_persistent_storage (cookie_manager, cookies_path,
                                                WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);

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
  GigWindow *window = NULL;
  GigPage *page = NULL;

  g_assert (GIG_IS_APPLICATION (application));

  window = find_or_create_window (GIG_APPLICATION (application));

  for (gint i = 0; i < n_files; i++)
    {
      g_autofree gchar *uri = g_file_get_uri (files[i]);
      GigWebView *web_view = NULL;
      gboolean set_selected = (i == n_files - 1);

      web_view = GIG_WEB_VIEW (gig_web_view_new ());
      gig_web_view_load_address (web_view, uri);

      page = gig_page_new (web_view);
      gig_window_add_tab_page (window, page, set_selected, NULL);
    }

  gtk_window_present (GTK_WINDOW (window));
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
