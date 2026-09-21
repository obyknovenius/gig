#include "gig-find-bar-private.h"

void
gig_find_bar_actions_previous_cb (GtkWidget *widget,
                                  const gchar *action_name,
                                  GVariant *param)
{
  GigFindBar *self = (GigFindBar *) widget;

  g_assert (GIG_IS_FIND_BAR (self));

  webkit_find_controller_search_previous (self->find_controller);
}

void
gig_find_bar_actions_next_cb (GtkWidget *widget,
                              const gchar *action_name,
                              GVariant *param)
{
  GigFindBar *self = (GigFindBar *) widget;

  g_assert (GIG_IS_FIND_BAR (self));

  webkit_find_controller_search_next (self->find_controller);
}

void
gig_find_bar_actions_dismiss_cb (GtkWidget *widget,
                                 const gchar *action_name,
                                 GVariant *param)
{
  GigFindBar *self = (GigFindBar *) widget;
  GtkWidget *toolbar_view = NULL;

  g_assert (GIG_IS_FIND_BAR (self));

  gtk_editable_set_text (GTK_EDITABLE (self->search_entry), "");

  webkit_find_controller_search_finish (self->find_controller);

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "find.previous", FALSE);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "find.next", FALSE);

  if ((toolbar_view = gtk_widget_get_ancestor (widget, ADW_TYPE_TOOLBAR_VIEW)))
    adw_toolbar_view_set_reveal_bottom_bars (ADW_TOOLBAR_VIEW (toolbar_view), FALSE);
}

void
gig_find_bar_class_actions_init (GigFindBarClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_install_action (widget_class, "find.previous", NULL,
                                   gig_find_bar_actions_previous_cb);

  gtk_widget_class_install_action (widget_class, "find.next", NULL,
                                   gig_find_bar_actions_next_cb);

  gtk_widget_class_install_action (widget_class, "find.dismiss", NULL,
                                   gig_find_bar_actions_dismiss_cb);
}

void
gig_find_bar_actions_init (GigFindBar *self)
{
  g_assert (GIG_IS_FIND_BAR (self));

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "find.previous", FALSE);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "find.next", FALSE);
}
