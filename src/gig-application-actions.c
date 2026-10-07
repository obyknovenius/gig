#include "config.h"

#include "gig-application-private.h"

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
    { "about", gig_application_actions_about_cb },
  };

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   actions,
                                   G_N_ELEMENTS (actions),
                                   self);
}
