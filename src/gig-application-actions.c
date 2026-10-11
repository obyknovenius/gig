#include "config.h"

#include "gig-application-private.h"

#include "gig-window.h"

static void
gig_application_actions_new_window_cb (GSimpleAction *action,
                                       GVariant *parameter,
                                       gpointer user_data)
{
  GigApplication *self = user_data;
  GigWindow *window;

  g_assert (GIG_IS_APPLICATION (self));

  window = gig_window_new (self);

  gtk_window_present (GTK_WINDOW (window));
}

static void
gig_application_actions_about_cb (GSimpleAction *action,
                                  GVariant *parameter,
                                  gpointer user_data)
{
  GigApplication *self = user_data;
  GigWindow *window;
  g_autofree gchar *debug_info = NULL;

  g_assert (GIG_IS_APPLICATION (self));

  window = gig_application_get_current_window (self);

  debug_info = g_strdup_printf ("WebKitGTK %d.%d.%d" WEBKIT_REVISION "\n"
                                "GTK %d.%d.%d\n"
                                "Libadwaita %d.%d.%d",
                                webkit_get_major_version (),
                                webkit_get_minor_version (),
                                webkit_get_micro_version (),
                                gtk_get_major_version (),
                                gtk_get_minor_version (),
                                gtk_get_micro_version (),
                                adw_get_major_version (),
                                adw_get_minor_version (),
                                adw_get_micro_version ());

  adw_show_about_dialog (GTK_WIDGET (window),
                         "application-name", "Side Gig",
                         "application-icon", APP_ID,
                         "version", PACKAGE_VERSION,
                         "developer-name", "Vitaly Dyachkov",
                         "debug-info", debug_info,
                         NULL);
}

void
gig_application_init_actions (GigApplication *self)
{
  static const GActionEntry actions[] = {
    { "new-window", gig_application_actions_new_window_cb },
    { "about", gig_application_actions_about_cb },
  };

  static const char *new_window_accels[] = { "<Control>n", NULL };

  g_assert (GIG_IS_APPLICATION (self));

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   actions,
                                   G_N_ELEMENTS (actions),
                                   self);

  gtk_application_set_accels_for_action (GTK_APPLICATION (self),
                                         "app.new-window",
                                         new_window_accels);
}
