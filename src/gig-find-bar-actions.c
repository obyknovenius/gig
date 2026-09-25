#include "gig-find-bar-private.h"

#include "gig-find-entry.h"

void
gig_find_bar_actions_previous_cb (GtkWidget *widget,
                                  const gchar *action_name,
                                  GVariant *param)
{
  GigFindBar *self = (GigFindBar *) widget;
  guint count, position;

  g_assert (GIG_IS_FIND_BAR (self));

  webkit_find_controller_search_previous (self->find_controller);

  count = gig_find_entry_get_occurrence_count (self->entry);
  position = gig_find_entry_get_occurrence_position (self->entry);

  gig_find_entry_set_occurrence_position (self->entry,
                                          position == 1 ? count : position - 1);
}

void
gig_find_bar_actions_next_cb (GtkWidget *widget,
                              const gchar *action_name,
                              GVariant *param)
{
  GigFindBar *self = (GigFindBar *) widget;
  guint count, position;

  g_assert (GIG_IS_FIND_BAR (self));

  webkit_find_controller_search_next (self->find_controller);

  count = gig_find_entry_get_occurrence_count (self->entry);
  position = gig_find_entry_get_occurrence_position (self->entry);

  gig_find_entry_set_occurrence_position (self->entry,
                                          position == count ? 1 : position + 1);
}

void
gig_find_bar_class_actions_init (GigFindBarClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_install_action (widget_class, "find.previous", NULL,
                                   gig_find_bar_actions_previous_cb);

  gtk_widget_class_install_action (widget_class, "find.next", NULL,
                                   gig_find_bar_actions_next_cb);
}

void
gig_find_bar_actions_init (GigFindBar *self)
{
  g_assert (GIG_IS_FIND_BAR (self));

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "find.previous", FALSE);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "find.next", FALSE);
}
