#include "gig-window-private.h"

static void
gig_window_actions_go_back_cb (GtkWidget *widget,
                               const char *action_name,
                               GVariant *param)
{
  GigWindow *self = (GigWindow *)widget;

  g_assert (GIG_IS_WINDOW (self));

  webkit_web_view_go_back (self->current_web_view);
}

static void
gig_window_actions_go_forward_cb (GtkWidget *widget,
                                  const char *action_name,
                                  GVariant *param)
{
  GigWindow *self = (GigWindow *)widget;

  g_assert (GIG_IS_WINDOW (self));

  webkit_web_view_go_forward (self->current_web_view);
}

static void
gig_window_actions_stop_reload_cb (GtkWidget *widget,
                                   const char *action_name,
                                   GVariant *param)
{
  GigWindow *self = (GigWindow *)widget;
  gboolean is_loading;

  g_assert (GIG_IS_WINDOW (self));

  is_loading = webkit_web_view_is_loading (self->current_web_view);

  if (is_loading)
    webkit_web_view_stop_loading (self->current_web_view);
  else
    webkit_web_view_reload (self->current_web_view);
}

static void
gig_window_actions_new_tab_cb (GtkWidget *widget,
                               const char *action_name,
                               GVariant *param)
{
  GigWindow *self = (GigWindow *)widget;

  g_assert (GIG_IS_WINDOW (self));

  gig_window_add_tab (self, "https://www.google.com");
}

void
gig_window_class_init_actions (GigWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_install_action (widget_class,
                                   "win.go-back",
                                   NULL,
                                   gig_window_actions_go_back_cb);
  gtk_widget_class_install_action (widget_class,
                                   "win.go-forward",
                                   NULL,
                                   gig_window_actions_go_forward_cb);
  gtk_widget_class_install_action (widget_class,
                                   "win.stop-reload",
                                   NULL,
                                   gig_window_actions_stop_reload_cb);
  gtk_widget_class_install_action (widget_class,
                                   "win.new-tab",
                                   NULL,
                                   gig_window_actions_new_tab_cb);
}

void
gig_window_update_actions (GigWindow *self,
                           WebKitWebView *web_view)
{
  gboolean can_go_back = FALSE;
  gboolean can_go_forward = FALSE;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (!web_view || WEBKIT_IS_WEB_VIEW (web_view));

  if (web_view)
    {
      can_go_back = webkit_web_view_can_go_back (web_view);
      can_go_forward = webkit_web_view_can_go_forward (web_view);
    }

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-back", can_go_back);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-forward", can_go_forward);
}
