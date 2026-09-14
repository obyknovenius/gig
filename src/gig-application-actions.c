#include "gig-application-private.h"

static void
gig_application_actions_about_cb (GSimpleAction *action,
                                  GVariant *parameter,
                                  gpointer user_data)
{
  GigApplication *self = user_data;
  GigWindow *window = NULL;

  g_assert (GIG_IS_APPLICATION (self));

  window = gig_application_get_current_window (self);

  adw_show_about_dialog (GTK_WIDGET (window),
                         "application-name", "Side Gig",
                         "application-icon", "web-browser",
                         "developer-name", "Vitaly Dyachkov",
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
