#include "gig-window-private.h"

#include "gig-address-bar-private.h"
#include "gig-find-bar.h"
#include "gig-tab.h"
#include "gig-web-view.h"

static void
gig_window_actions_new_tab_cb (GtkWidget *widget,
                               const gchar *action_name,
                               GVariant *param)
{
  GigWindow *self = (GigWindow *) widget;
  GigTab *tab = NULL;

  g_assert (GIG_IS_WINDOW (self));

  tab = GIG_TAB (gig_tab_new ());

  gig_window_add_tab (self, tab, TRUE, NULL);
}

static void
gig_window_actions_stop_reload_cb (GtkWidget *widget,
                                   const gchar *action_name,
                                   GVariant *param)
{
  GigWindow *self = (GigWindow *) widget;
  GigTab *tab;
  GigWebView *web_view;

  g_assert (GIG_IS_WINDOW (self));

  tab = gig_window_get_selected_tab (self);
  g_assert (GIG_IS_TAB (tab));

  web_view = gig_tab_get_web_view (tab);
  g_assert (GIG_IS_WEB_VIEW (web_view));

  if (webkit_web_view_is_loading (WEBKIT_WEB_VIEW (web_view)))
    webkit_web_view_stop_loading (WEBKIT_WEB_VIEW (web_view));
  else
    {
      gig_address_bar_reset (self->address_bar);
      webkit_web_view_reload (WEBKIT_WEB_VIEW (web_view));
    }
}

static void
gig_window_actions_go_back_cb (GtkWidget *widget,
                               const gchar *action_name,
                               GVariant *param)
{
  GigWindow *self = (GigWindow *) widget;
  GigTab *tab;
  GigWebView *web_view;

  g_assert (GIG_IS_WINDOW (self));

  tab = gig_window_get_selected_tab (self);
  g_assert (GIG_IS_TAB (tab));

  web_view = gig_tab_get_web_view (tab);
  g_assert (GIG_IS_WEB_VIEW (web_view));

  webkit_web_view_go_back (WEBKIT_WEB_VIEW (web_view));
}

static void
gig_window_actions_go_forward_cb (GtkWidget *widget,
                                  const gchar *action_name,
                                  GVariant *param)
{
  GigWindow *self = (GigWindow *) widget;
  GigTab *tab;
  GigWebView *web_view;

  g_assert (GIG_IS_WINDOW (self));

  tab = gig_window_get_selected_tab (self);
  g_assert (GIG_IS_TAB (tab));

  web_view = gig_tab_get_web_view (tab);
  g_assert (GIG_IS_WEB_VIEW (web_view));

  webkit_web_view_go_forward (WEBKIT_WEB_VIEW (web_view));
}

static void
gig_window_actions_tab_find_cb (GtkWidget *widget,
                                const gchar *action_name,
                                GVariant *param)
{
  GigWindow *self = (GigWindow *) widget;
  GigTab *tab;

  g_assert (GIG_IS_WINDOW (self));

  tab = gig_window_get_selected_tab (self);
  g_assert (GIG_IS_TAB (tab));

  gig_tab_reveal_find_bar (tab);
}

static void
gig_window_actions_tab_find_finish_cb (GtkWidget *widget,
                                       const gchar *action_name,
                                       GVariant *param)
{
  GigWindow *self = (GigWindow *) widget;
  GigTab *tab;

  g_assert (GIG_IS_WINDOW (self));

  tab = gig_window_get_selected_tab (self);
  g_assert (GIG_IS_TAB (tab));

  gig_tab_dismiss_find_bar (tab);
}

static void
gig_window_actions_tab_close_cb (GtkWidget *widget,
                                 const gchar *action_name,
                                 GVariant *param)
{
  GigWindow *self = (GigWindow *) widget;
  AdwTabPage *tab_page;

  g_assert (GIG_IS_WINDOW (self));

  tab_page = self->menu_page;
  if (!tab_page)
    tab_page = adw_tab_view_get_selected_page (self->tab_view);

  adw_tab_view_close_page (self->tab_view, tab_page);
}

void
gig_window_class_actions_init (GigWindowClass *klass)
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

  gtk_widget_class_install_action (widget_class, "tab.find", NULL,
                                   gig_window_actions_tab_find_cb);

  gtk_widget_class_install_action (widget_class, "tab.find-finish", NULL,
                                   gig_window_actions_tab_find_finish_cb);

  gtk_widget_class_install_action (widget_class, "tab.close", NULL,
                                   gig_window_actions_tab_close_cb);

  gtk_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_t, GDK_CONTROL_MASK,
                                       "win.new-tab", NULL);

  gtk_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_f, GDK_CONTROL_MASK,
                                       "tab.find", NULL);

  gtk_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Escape, GDK_NO_MODIFIER_MASK,
                                       "tab.find-finish", NULL);

  gtk_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_w, GDK_CONTROL_MASK,
                                       "tab.close", NULL);
}

void
gig_window_actions_init (GigWindow *self)
{
  g_assert (GIG_IS_WINDOW (self));

  gig_window_actions_update (self);
}

void
gig_window_actions_update (GigWindow *self)
{
  GigTab *tab = NULL;
  gboolean is_blank = TRUE;
  gboolean can_go_back = FALSE;
  gboolean can_go_forward = FALSE;

  g_assert (GIG_IS_WINDOW (self));

  if ((tab = gig_window_get_selected_tab (self)))
    {
      GigWebView *web_view = gig_tab_get_web_view (tab);
      is_blank = gig_web_view_is_blank (web_view);
      can_go_back = webkit_web_view_can_go_back (WEBKIT_WEB_VIEW (web_view));
      can_go_forward = webkit_web_view_can_go_forward (WEBKIT_WEB_VIEW (web_view));
    }

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.stop-reload", !is_blank);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-back", can_go_back);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-forward", can_go_forward);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "tab.find", !is_blank);
}
