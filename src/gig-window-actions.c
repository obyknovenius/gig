#include "gig-window-private.h"

#include "gig-address-bar-private.h"
#include "gig-find-bar.h"
#include "gig-page.h"
#include "gig-web-view.h"

static void
gig_window_actions_new_tab_cb (GtkWidget *widget,
                               const gchar *action_name,
                               GVariant *param)
{
  GigWindow *self = (GigWindow *) widget;
  GigPage *page = NULL;

  g_assert (GIG_IS_WINDOW (self));

  page = GIG_PAGE (gig_page_new ());

  gig_window_add_page (self, page, TRUE, NULL);
}

static void
gig_window_actions_stop_reload_cb (GtkWidget *widget,
                                   const gchar *action_name,
                                   GVariant *param)
{
  GigWindow *self = (GigWindow *) widget;
  GigPage *page;
  GigWebView *web_view;

  g_assert (GIG_IS_WINDOW (self));

  page = gig_window_get_selected_page (self);
  g_assert (GIG_IS_PAGE (page));

  web_view = gig_page_get_web_view (page);
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
  GigPage *page;
  GigWebView *web_view;

  g_assert (GIG_IS_WINDOW (self));

  page = gig_window_get_selected_page (self);
  g_assert (GIG_IS_PAGE (page));

  web_view = gig_page_get_web_view (page);
  g_assert (GIG_IS_WEB_VIEW (web_view));

  webkit_web_view_go_back (WEBKIT_WEB_VIEW (web_view));
}

static void
gig_window_actions_go_forward_cb (GtkWidget *widget,
                                  const gchar *action_name,
                                  GVariant *param)
{
  GigWindow *self = (GigWindow *) widget;
  GigPage *page;
  GigWebView *web_view;

  g_assert (GIG_IS_WINDOW (self));

  page = gig_window_get_selected_page (self);
  g_assert (GIG_IS_PAGE (page));

  web_view = gig_page_get_web_view (page);
  g_assert (GIG_IS_WEB_VIEW (web_view));

  webkit_web_view_go_forward (WEBKIT_WEB_VIEW (web_view));
}

static void
gig_window_actions_page_find_cb (GtkWidget *widget,
                                 const gchar *action_name,
                                 GVariant *param)
{
  GigWindow *self = (GigWindow *) widget;
  GigPage *page;

  g_assert (GIG_IS_WINDOW (self));

  page = gig_window_get_selected_page (self);
  g_assert (GIG_IS_PAGE (page));

  gig_page_reveal_find_bar (page);
}

static void
gig_window_actions_page_find_finish_cb (GtkWidget *widget,
                                        const gchar *action_name,
                                        GVariant *param)
{
  GigWindow *self = (GigWindow *) widget;
  GigPage *page;

  g_assert (GIG_IS_WINDOW (self));

  page = gig_window_get_selected_page (self);
  g_assert (GIG_IS_PAGE (page));

  gig_page_dismiss_find_bar (page);
}

static void
gig_window_actions_page_close_cb (GtkWidget *widget,
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

  gtk_widget_class_install_action (widget_class, "page.find", NULL,
                                   gig_window_actions_page_find_cb);

  gtk_widget_class_install_action (widget_class, "page.find-finish", NULL,
                                   gig_window_actions_page_find_finish_cb);

  gtk_widget_class_install_action (widget_class, "page.close", NULL,
                                   gig_window_actions_page_close_cb);

  gtk_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_t, GDK_CONTROL_MASK,
                                       "win.new-tab", NULL);

  gtk_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_f, GDK_CONTROL_MASK,
                                       "page.find", NULL);

  gtk_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Escape, GDK_NO_MODIFIER_MASK,
                                       "page.find-finish", NULL);

  gtk_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_w, GDK_CONTROL_MASK,
                                       "page.close", NULL);
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
  GigPage *page = NULL;
  gboolean is_blank = TRUE;
  gboolean can_go_back = FALSE;
  gboolean can_go_forward = FALSE;

  g_assert (GIG_IS_WINDOW (self));

  if ((page = gig_window_get_selected_page (self)))
    {
      GigWebView *web_view = gig_page_get_web_view (page);
      is_blank = gig_web_view_is_blank (web_view);
      can_go_back = webkit_web_view_can_go_back (WEBKIT_WEB_VIEW (web_view));
      can_go_forward = webkit_web_view_can_go_forward (WEBKIT_WEB_VIEW (web_view));
    }

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.stop-reload", !is_blank);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-back", can_go_back);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-forward", can_go_forward);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "page.find", !is_blank);
}
