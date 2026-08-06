#include "gig-window-private.h"

G_DEFINE_TYPE (GigWindow, gig_window, ADW_TYPE_APPLICATION_WINDOW)

static void
url_entry_activate_cb (GigWindow *self,
                       GtkEntry *entry)
{
  WebKitWebView *web_view;
  const char *uri;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GTK_IS_ENTRY (entry));

  web_view = self->current_web_view;
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  uri = gtk_editable_get_text (GTK_EDITABLE (entry));

  webkit_web_view_load_uri (web_view, uri);
}

static void
web_view_notify_uri_cb (GigWindow *self,
                        GParamSpec *pspec,
                        WebKitWebView *web_view)
{
  const char *uri;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  uri = webkit_web_view_get_uri (web_view);
  gtk_editable_set_text (GTK_EDITABLE (self->url_entry), uri ? uri : "");
}

static void
web_view_notify_title_cb (GigWindow *self,
                          GParamSpec *pspec,
                          WebKitWebView *web_view)
{
  AdwTabPage *tab_page;
  const char *title;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  tab_page = adw_tab_view_get_page (self->tab_view, GTK_WIDGET (web_view));
  g_assert (ADW_IS_TAB_PAGE (tab_page));

  title = webkit_web_view_get_title (web_view);
  adw_tab_page_set_title (tab_page, title ? title : "Untitled");
}

static void
back_forward_list_changed_cb (GigWindow *self,
                              WebKitBackForwardList *back_forward_list)
{
  WebKitWebView *web_view;
  gboolean can_go_back;
  gboolean can_go_forward;

  g_assert (GIG_IS_WINDOW (self));

  web_view = self->current_web_view;
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  can_go_back = webkit_web_view_can_go_back (web_view);
  can_go_forward = webkit_web_view_can_go_forward (web_view);

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-back", can_go_back);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-forward", can_go_forward);
}

static void
web_view_notify_is_loading_cb (GigWindow *self,
                               GParamSpec *pspec,
                               WebKitWebView *web_view)
{
  gboolean is_loading;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  is_loading = webkit_web_view_is_loading (web_view);

  gtk_button_set_icon_name (GTK_BUTTON (self->stop_reload_button),
                            is_loading ? "process-stop-symbolic"
                                       : "view-refresh-symbolic");
}

static void
tab_view_notify_selected_page_cb (GigWindow *self,
                                  GParamSpec *pspec,
                                  AdwTabView *tab_view)
{
  AdwTabPage *tab_page;
  WebKitWebView *web_view = NULL;
  WebKitBackForwardList *back_forward_list = NULL;
  const char *uri = NULL;
  gboolean is_loading = FALSE;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (ADW_IS_TAB_VIEW (tab_view));

  if ((tab_page = adw_tab_view_get_selected_page (tab_view)))
    web_view = WEBKIT_WEB_VIEW (adw_tab_page_get_child (tab_page));

  g_assert (!web_view || WEBKIT_IS_WEB_VIEW (web_view));

  gtk_widget_set_sensitive (GTK_WIDGET (self->url_entry), web_view != NULL);

  if (web_view)
    {
      back_forward_list = webkit_web_view_get_back_forward_list (web_view);
      uri = webkit_web_view_get_uri (web_view);
      is_loading = webkit_web_view_is_loading (web_view);
    }

  gtk_editable_set_text (GTK_EDITABLE (self->url_entry), uri ? uri : "");

  gtk_button_set_icon_name (GTK_BUTTON (self->stop_reload_button),
                            is_loading ? "process-stop-symbolic"
                                       : "view-refresh-symbolic");

  self->current_web_view = web_view;

  gig_window_update_actions (self, web_view);

  g_signal_group_set_target (self->web_view_signals, web_view);
  g_signal_group_set_target (self->back_forward_list_signals, back_forward_list);
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
  GigWindow *self = (GigWindow *)object;

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

  gtk_widget_class_bind_template_callback (widget_class, url_entry_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, tab_view_notify_selected_page_cb);

  gig_window_class_init_actions (klass);

  g_type_ensure (WEBKIT_TYPE_WEB_VIEW);
}

static void
gig_window_init (GigWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->web_view_signals = g_signal_group_new (WEBKIT_TYPE_WEB_VIEW);

  g_signal_group_connect_object (self->web_view_signals,
                                 "notify::uri",
                                 G_CALLBACK (web_view_notify_uri_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

  g_signal_group_connect_object (self->web_view_signals,
                                 "notify::title",
                                 G_CALLBACK (web_view_notify_title_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

  g_signal_group_connect_object (self->web_view_signals,
                                 "notify::is-loading",
                                 G_CALLBACK (web_view_notify_is_loading_cb),
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

void
gig_window_add_tab (GigWindow *self)
{
  AdwTabPage *tab_page;
  WebKitWebView *web_view;

  g_return_if_fail (GIG_IS_WINDOW (self));

  web_view = WEBKIT_WEB_VIEW (webkit_web_view_new ());
  tab_page = adw_tab_view_append (self->tab_view, GTK_WIDGET (web_view));

  adw_tab_page_set_title (tab_page, "New Tab");

  adw_tab_view_set_selected_page (self->tab_view, tab_page);
}
