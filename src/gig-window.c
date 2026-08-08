#include "gig-window-private.h"

#include "gig-page.h"
#include "gig-utils.h"

G_DEFINE_TYPE (GigWindow, gig_window, ADW_TYPE_APPLICATION_WINDOW)

static void
url_entry_activate_cb (GigWindow *self,
                       GtkEntry *entry)
{
  WebKitWebView *web_view;
  const gchar *text;
  g_autofree gchar *uri = NULL;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GTK_IS_ENTRY (entry));
  g_assert (GIG_IS_PAGE (self->selected_page));

  text = gtk_editable_get_text (GTK_EDITABLE (entry));
  if (!text || text[0] == '\0')
    return;

  uri = gig_utils_fixup_uri (text);
  if (!uri)
    uri = gig_utils_build_search_uri (text);

  web_view = gig_page_get_web_view (self->selected_page);
  webkit_web_view_load_uri (web_view, uri);
}

static void
set_selected_page (GigWindow *self,
                   GigPage *page)
{
  WebKitWebView *web_view = NULL;
  WebKitBackForwardList *back_forward_list = NULL;
  const gchar *uri = NULL;
  bool is_loading = FALSE;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (!page || GIG_IS_PAGE (page));

  if (self->selected_page == page)
    return;

  gtk_widget_set_sensitive (GTK_WIDGET (self->url_entry), page != NULL);

  if (page)
    {
      web_view = gig_page_get_web_view (page);
      back_forward_list = webkit_web_view_get_back_forward_list (web_view);
      uri = webkit_web_view_get_uri (web_view);
      is_loading = webkit_web_view_is_loading (web_view);
    }

  gtk_editable_set_text (GTK_EDITABLE (self->url_entry), uri ? uri : "");

  gtk_button_set_icon_name (GTK_BUTTON (self->stop_reload_button),
                            is_loading ? "process-stop-symbolic"
                                       : "view-refresh-symbolic");

  g_signal_group_set_target (self->web_view_signals, web_view);
  g_signal_group_set_target (self->back_forward_list_signals, back_forward_list);

  gig_window_update_actions (self, web_view);

  self->selected_page = page;
}

static void
web_view_notify_uri_cb (GigWindow *self,
                        GParamSpec *pspec,
                        WebKitWebView *web_view)
{
  const gchar *uri;
  gboolean can_stop_reload;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  uri = webkit_web_view_get_uri (web_view);
  gtk_editable_set_text (GTK_EDITABLE (self->url_entry), uri ? uri : "");

  can_stop_reload = uri != NULL;
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.stop-reload", can_stop_reload);
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
back_forward_list_changed_cb (GigWindow *self,
                              WebKitBackForwardListItem *item_added,
                              gpointer items_removed,
                              WebKitBackForwardList *back_forward_list)
{
  WebKitWebView *web_view;
  gboolean can_go_back;
  gboolean can_go_forward;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_BACK_FORWARD_LIST (back_forward_list));
  g_assert (GIG_IS_PAGE (self->selected_page));

  web_view = gig_page_get_web_view (self->selected_page);

  g_assert (webkit_web_view_get_back_forward_list (web_view) == back_forward_list);

  can_go_back = webkit_web_view_can_go_back (web_view);
  can_go_forward = webkit_web_view_can_go_forward (web_view);

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-back", can_go_back);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-forward", can_go_forward);
}

static void
tab_view_notify_selected_page_cb (GigWindow *self,
                                  GParamSpec *pspec,
                                  AdwTabView *tab_view)
{
  AdwTabPage *tab_page;
  GigPage *page = NULL;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (ADW_IS_TAB_VIEW (tab_view));

  if ((tab_page = adw_tab_view_get_selected_page (tab_view)))
    page = GIG_PAGE (adw_tab_page_get_child (tab_page));

  set_selected_page (self, page);
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

  gtk_widget_class_bind_template_child (widget_class, GigWindow, url_entry);
  gtk_widget_class_bind_template_child (widget_class, GigWindow, stop_reload_button);
  gtk_widget_class_bind_template_child (widget_class, GigWindow, tab_view);

  gtk_widget_class_bind_template_callback (widget_class, url_entry_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, tab_view_notify_selected_page_cb);

  gig_window_class_init_actions (klass);
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
gig_window_add_page (GigWindow *self,
                     GigPage *page)
{
  AdwTabPage *tab_page;
  WebKitWebView *web_view;

  g_return_if_fail (GIG_IS_WINDOW (self));
  g_return_if_fail (GIG_IS_PAGE (page));

  web_view = gig_page_get_web_view (page);

  tab_page = adw_tab_view_append (self->tab_view, GTK_WIDGET (page));

  g_object_bind_property (page, "title", tab_page, "title", G_BINDING_SYNC_CREATE);
  g_object_bind_property (web_view, "is-loading", tab_page, "loading", G_BINDING_SYNC_CREATE);

  adw_tab_view_set_selected_page (self->tab_view, tab_page);
}
