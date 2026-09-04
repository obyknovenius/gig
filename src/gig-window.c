#include "gig-window-private.h"

#include "gig-page.h"
#include "gig-url-entry.h"

G_DEFINE_TYPE (GigWindow, gig_window, ADW_TYPE_APPLICATION_WINDOW)

static AdwTabPage *
tab_overview_create_tab_cb (GigWindow *self,
                            AdwTabOverview *tab_overview)
{
  GigPage *page = NULL;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (ADW_IS_TAB_OVERVIEW (tab_overview));

  page = gig_page_new ();

  return gig_window_add_page (self, page);
}

static void
web_view_ready_to_show_cb (GigWindow *self,
                           WebKitWebView *web_view)
{
  GigPage *page = NULL;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  g_signal_handlers_disconnect_by_func (web_view, web_view_ready_to_show_cb, self);

  page = GIG_PAGE (gtk_widget_get_ancestor (GTK_WIDGET (web_view), GIG_TYPE_PAGE));
  g_assert (GIG_IS_PAGE (page));

  gig_window_add_page (self, page);
}

static WebKitWebView *
web_view_create_cb (GigWindow *self,
                    WebKitNavigationAction *navigation_action,
                    WebKitWebView *related_web_view)
{
  GigPage *page = NULL;
  WebKitWebView *web_view = NULL;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (related_web_view));

  page = gig_page_new_with_related_web_view (related_web_view);
  web_view = gig_page_get_web_view (page);

  g_signal_connect_object (web_view,
                           "ready-to-show",
                           G_CALLBACK (web_view_ready_to_show_cb),
                           self,
                           G_CONNECT_SWAPPED);

  return web_view;
}

static void
web_view_uri_changed_cb (GigWindow *self,
                         GParamSpec *pspec,
                         WebKitWebView *web_view)
{
  const gchar *uri = NULL;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  uri = webkit_web_view_get_uri (web_view);

  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "win.stop-reload",
                                 uri != NULL);
}

static void
web_view_is_loading_changed_cb (GigWindow *self,
                                GParamSpec *pspec,
                                WebKitWebView *web_view)
{
  gboolean is_loading = FALSE;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  is_loading = webkit_web_view_is_loading (web_view);

  gtk_button_set_icon_name (GTK_BUTTON (self->stop_reload_button),
                            is_loading ? "process-stop-symbolic"
                                       : "view-refresh-symbolic");
}

static void
back_forward_list_changed_cb (GigWindow *self,
                              WebKitBackForwardListItem *item_added,
                              gpointer items_removed,
                              WebKitBackForwardList *back_forward_list)
{
  GList *back_list = NULL;
  GList *forward_list = NULL;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_BACK_FORWARD_LIST (back_forward_list));

  back_list = webkit_back_forward_list_get_back_list (back_forward_list);
  forward_list = webkit_back_forward_list_get_forward_list (back_forward_list);

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-back",
                                 back_list != NULL);

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-forward",
                                 forward_list != NULL);

  g_list_free (back_list);
  g_list_free (forward_list);
}

static void
tab_view_selected_page_changed_cb (GigWindow *self,
                                   GParamSpec *pspec,
                                   AdwTabView *tab_view)
{
  AdwTabPage *tab_page = NULL;
  GigPage *page = NULL;
  WebKitWebView *web_view = NULL;
  WebKitBackForwardList *back_forward_list = NULL;
  gboolean is_loading = FALSE;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (ADW_IS_TAB_VIEW (tab_view));

  if ((tab_page = adw_tab_view_get_selected_page (tab_view)))
    page = GIG_PAGE (adw_tab_page_get_child (tab_page));

  if (self->selected_page == page)
    return;

  if (page)
    {
      web_view = gig_page_get_web_view (page);
      g_assert (WEBKIT_IS_WEB_VIEW (web_view));

      back_forward_list = webkit_web_view_get_back_forward_list (web_view);
      is_loading = webkit_web_view_is_loading (web_view);
    }

  gtk_button_set_icon_name (GTK_BUTTON (self->stop_reload_button),
                            is_loading ? "process-stop-symbolic"
                                       : "view-refresh-symbolic");

  gig_url_entry_set_web_view (GIG_URL_ENTRY (self->url_entry), web_view);

  gig_window_update_actions (self, web_view);

  g_signal_group_set_target (self->web_view_signals, web_view);
  g_signal_group_set_target (self->back_forward_list_signals, back_forward_list);

  self->selected_page = page;
}

static void
gig_window_dispose (GObject *object)
{
  GigWindow *self = GIG_WINDOW (object);

  g_assert (GIG_IS_WINDOW (self));

  g_signal_group_set_target (self->web_view_signals, NULL);
  g_signal_group_set_target (self->back_forward_list_signals, NULL);

  G_OBJECT_CLASS (gig_window_parent_class)->dispose (object);
}

static void
gig_window_finalize (GObject *object)
{
  GigWindow *self = (GigWindow *) object;

  g_assert (GIG_IS_WINDOW (self));

  g_clear_object (&self->web_view_signals);
  g_clear_object (&self->back_forward_list_signals);

  G_OBJECT_CLASS (gig_window_parent_class)->finalize (object);
}

static void
gig_window_class_init (GigWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = gig_window_dispose;
  object_class->finalize = gig_window_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/obyknovenius/Gig/ui/gig-window.ui");

  gtk_widget_class_bind_template_child (widget_class, GigWindow, stop_reload_button);
  gtk_widget_class_bind_template_child (widget_class, GigWindow, url_entry);
  gtk_widget_class_bind_template_child (widget_class, GigWindow, tab_view);

  gtk_widget_class_bind_template_callback (widget_class, tab_overview_create_tab_cb);
  gtk_widget_class_bind_template_callback (widget_class, tab_view_selected_page_changed_cb);

  g_type_ensure (GIG_TYPE_URL_ENTRY);

  gig_window_class_init_actions (klass);
}

static void
gig_window_init (GigWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->web_view_signals = g_signal_group_new (WEBKIT_TYPE_WEB_VIEW);

  g_signal_group_connect_object (self->web_view_signals,
                                 "create",
                                 G_CALLBACK (web_view_create_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

  g_signal_group_connect_object (self->web_view_signals,
                                 "notify::uri",
                                 G_CALLBACK (web_view_uri_changed_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

  g_signal_group_connect_object (self->web_view_signals,
                                 "notify::is-loading",
                                 G_CALLBACK (web_view_is_loading_changed_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

  self->back_forward_list_signals = g_signal_group_new (WEBKIT_TYPE_BACK_FORWARD_LIST);

  g_signal_group_connect_object (self->back_forward_list_signals,
                                 "changed",
                                 G_CALLBACK (back_forward_list_changed_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

  gig_window_init_actions (self);
}

GigWindow *
gig_window_new (GtkApplication *application)
{
  g_return_val_if_fail (G_APPLICATION (application), NULL);

  return g_object_new (GIG_TYPE_WINDOW,
                       "application", application,
                       NULL);
}

AdwTabPage *
gig_window_add_page (GigWindow *self,
                     GigPage *page)
{
  AdwTabPage *tab_page;

  g_return_val_if_fail (GIG_IS_WINDOW (self), NULL);
  g_return_val_if_fail (GIG_IS_PAGE (page), NULL);

  tab_page = adw_tab_view_append (self->tab_view, GTK_WIDGET (page));

  g_object_bind_property (page, "title",
                          tab_page, "title",
                          G_BINDING_SYNC_CREATE);

  g_object_bind_property (page, "icon",
                          tab_page, "icon",
                          G_BINDING_SYNC_CREATE);

  g_object_bind_property (page, "is-loading",
                          tab_page, "loading",
                          G_BINDING_SYNC_CREATE);

  return tab_page;
}
