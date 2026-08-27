#include "gig-window-private.h"

#include "gig-page.h"

static void
gig_window_actions_new_tab_cb (GtkWidget *widget,
                               const gchar *action_name,
                               GVariant *param)
{
  GigWindow *self = (GigWindow *) widget;
  GigPage *page;

  g_assert (GIG_IS_WINDOW (self));

  page = gig_page_new ();

  gig_window_add_page (self, page);
}

static void
gig_window_actions_stop_reload_cb (GtkWidget *widget,
                                   const gchar *action_name,
                                   GVariant *param)
{
  GigWindow *self = (GigWindow *) widget;
  gboolean is_loading;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_PAGE (self->selected_page));

  is_loading = gig_page_get_is_loading (self->selected_page);

  if (is_loading)
    gig_page_stop_loading (self->selected_page);
  else
    {
      const gchar *uri = gig_page_get_uri (self->selected_page);

      gtk_editable_set_text (GTK_EDITABLE (self->url_entry), uri ? uri : "");

      gtk_entry_set_icon_from_icon_name (GTK_ENTRY (self->url_entry),
                                         GTK_ENTRY_ICON_PRIMARY,
                                         uri ? "info-outline-symbolic"
                                             : "system-search-symbolic");

      gtk_entry_set_icon_from_icon_name (GTK_ENTRY (self->url_entry),
                                         GTK_ENTRY_ICON_SECONDARY,
                                         uri ? "user-bookmarks-symbolic" : NULL);

      gig_page_reload (self->selected_page);
    }
}

static void
gig_window_actions_go_back_cb (GtkWidget *widget,
                               const gchar *action_name,
                               GVariant *param)
{
  GigWindow *self = (GigWindow *) widget;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_PAGE (self->selected_page));

  gig_page_go_back (self->selected_page);
}

static void
gig_window_actions_go_forward_cb (GtkWidget *widget,
                                  const gchar *action_name,
                                  GVariant *param)
{
  GigWindow *self = (GigWindow *) widget;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_PAGE (self->selected_page));

  gig_page_go_forward (self->selected_page);
}

void
gig_window_class_init_actions (GigWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_install_action (widget_class, "win.new-tab", NULL,
                                   gig_window_actions_new_tab_cb);

  gtk_widget_class_install_action (widget_class, "win.stop-reload", NULL,
                                   gig_window_actions_stop_reload_cb);

  gtk_widget_class_install_action (widget_class, "win.go-back", NULL,
                                   gig_window_actions_go_back_cb);

  gtk_widget_class_install_action (widget_class, "win.go-forward", NULL,
                                   gig_window_actions_go_forward_cb);
}

void
gig_window_init_actions (GigWindow *self)
{
  g_assert (GIG_IS_WINDOW (self));

  gig_window_update_actions (self, NULL);
}

void
gig_window_update_actions (GigWindow *self,
                           GigPage *page)
{
  gboolean can_stop_reload = FALSE;
  gboolean can_go_back = FALSE;
  gboolean can_go_forward = FALSE;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (!page || GIG_IS_PAGE (page));

  if (page)
    {
      can_stop_reload = gig_page_get_uri (page) != NULL;
      can_go_back = gig_page_can_go_back (page);
      can_go_forward = gig_page_can_go_forward (page);
    }

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.stop-reload", can_stop_reload);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-back", can_go_back);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-forward", can_go_forward);
}
