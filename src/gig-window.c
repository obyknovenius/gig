#include "gig-window.h"

struct _GigWindow
{
  AdwApplicationWindow parent_instance;

  GtkButton *stop_reload_button;
  GtkEntry *url_entry;
  AdwTabView *tab_view;

  GSignalGroup *web_view_signals;
  GSignalGroup *back_forward_list_signals;
};

G_DEFINE_TYPE (GigWindow, gig_window, ADW_TYPE_APPLICATION_WINDOW)

static void
update_url_entry (GigWindow *self,
                  WebKitWebView *web_view)
{
  const char *uri;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  uri = webkit_web_view_get_uri (web_view);
  gtk_editable_set_text (GTK_EDITABLE (self->url_entry), uri ? uri : "");
}

static void
update_back_forward_actions (GigWindow *self,
                             WebKitWebView *web_view)
{
  GAction *action;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  action = g_action_map_lookup_action (G_ACTION_MAP (self), "go-back");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
                               webkit_web_view_can_go_back (web_view));

  action = g_action_map_lookup_action (G_ACTION_MAP (self), "go-forward");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
                               webkit_web_view_can_go_forward (web_view));
}

static void
update_stop_reload_action (GigWindow *self,
                           WebKitWebView *web_view)
{
  gboolean is_loading;
  GAction *action;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  is_loading = webkit_web_view_is_loading (web_view);

  action = g_action_map_lookup_action (G_ACTION_MAP (self), "stop-reload");
  g_simple_action_set_state (G_SIMPLE_ACTION (action),
                             g_variant_new_boolean (is_loading));

  gtk_button_set_icon_name (GTK_BUTTON (self->stop_reload_button),
                            is_loading ? "process-stop-symbolic"
                                       : "view-refresh-symbolic");
}

static void
gig_window_go_back (GSimpleAction *action,
                    GVariant *parameter,
                    gpointer user_data)
{
  GigWindow *self = (GigWindow *)user_data;
  AdwTabPage *tab_page;
  WebKitWebView *web_view;

  g_assert (GIG_IS_WINDOW (self));

  tab_page = adw_tab_view_get_selected_page (self->tab_view);
  web_view = WEBKIT_WEB_VIEW (adw_tab_page_get_child (tab_page));

  webkit_web_view_go_back (web_view);
}

static void
gig_window_go_forward (GSimpleAction *action,
                       GVariant *parameter,
                       gpointer user_data)
{
  GigWindow *self = (GigWindow *)user_data;
  AdwTabPage *tab_page;
  WebKitWebView *web_view;

  g_assert (GIG_IS_WINDOW (self));

  tab_page = adw_tab_view_get_selected_page (self->tab_view);
  web_view = WEBKIT_WEB_VIEW (adw_tab_page_get_child (tab_page));

  webkit_web_view_go_forward (web_view);
}

static void
gig_window_stop_reload (GSimpleAction *action,
                        GVariant *parameter,
                        gpointer user_data)
{
  GigWindow *self = (GigWindow *)user_data;
  AdwTabPage *tab_page;
  WebKitWebView *web_view;
  g_autoptr (GVariant) state = NULL;

  g_assert (GIG_IS_WINDOW (self));

  tab_page = adw_tab_view_get_selected_page (self->tab_view);
  web_view = WEBKIT_WEB_VIEW (adw_tab_page_get_child (tab_page));

  state = g_action_get_state (G_ACTION (action));
  if (g_variant_get_boolean (state))
    webkit_web_view_stop_loading (web_view);
  else
    webkit_web_view_reload (web_view);
}

static void
gig_window_new_tab (GSimpleAction *action,
                    GVariant *parameter,
                    gpointer user_data)
{
  GigWindow *self = (GigWindow *)user_data;

  g_assert (GIG_IS_WINDOW (self));

  gig_window_add_tab (self, "https://www.google.com");
}

static const GActionEntry actions[] = {
  { "go-back", gig_window_go_back },
  { "go-forward", gig_window_go_forward },
  { "stop-reload", gig_window_stop_reload, NULL, "false" },
  { "new-tab", gig_window_new_tab },
};

static void
url_entry_activate_cb (GigWindow *self,
                       GtkEntry *entry)
{
  AdwTabPage *tab_page;
  WebKitWebView *web_view;
  const char *uri;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GTK_IS_ENTRY (entry));

  uri = gtk_editable_get_text (GTK_EDITABLE (entry));

  tab_page = adw_tab_view_get_selected_page (self->tab_view);
  web_view = WEBKIT_WEB_VIEW (adw_tab_page_get_child (tab_page));

  webkit_web_view_load_uri (web_view, uri);
}

static void
web_view_notify_uri_cb (GigWindow *self,
                        GParamSpec *pspec,
                        WebKitWebView *web_view)
{
  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  update_url_entry (self, web_view);
}

static void
back_forward_list_changed_cb (GigWindow *self)
{
  AdwTabPage *tab_page;
  WebKitWebView *web_view;

  g_assert (GIG_IS_WINDOW (self));

  tab_page = adw_tab_view_get_selected_page (self->tab_view);
  web_view = WEBKIT_WEB_VIEW (adw_tab_page_get_child (tab_page));

  update_back_forward_actions (self, web_view);
}

static void
web_view_notify_is_loading_cb (GigWindow *self,
                               GParamSpec *pspec,
                               WebKitWebView *web_view)
{
  g_assert (GIG_IS_WINDOW (self));

  update_stop_reload_action (self, web_view);
}

static void
tab_view_notify_selected_page_cb (GigWindow *self,
                                  GParamSpec *pspec,
                                  AdwTabView *tab_view)
{
  AdwTabPage *tab_page;
  WebKitWebView *web_view;
  WebKitBackForwardList *back_forward_list;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (ADW_IS_TAB_VIEW (tab_view));

  tab_page = adw_tab_view_get_selected_page (tab_view);
  web_view = WEBKIT_WEB_VIEW (adw_tab_page_get_child (tab_page));

  update_url_entry (self, web_view);
  update_back_forward_actions (self, web_view);
  update_stop_reload_action (self, web_view);

  g_signal_group_set_target (self->web_view_signals, web_view);

  back_forward_list = webkit_web_view_get_back_forward_list (web_view);
  g_signal_group_set_target (self->back_forward_list_signals, back_forward_list);
}

static void
gig_window_constructed (GObject *object)
{
  GigWindow *self = GIG_WINDOW (object);
  GAction *action;

  g_assert (GIG_IS_WINDOW (self));

  G_OBJECT_CLASS (gig_window_parent_class)->constructed (object);

  g_action_map_add_action_entries (G_ACTION_MAP (self), actions,
                                   G_N_ELEMENTS (actions), self);

  action = g_action_map_lookup_action (G_ACTION_MAP (self), "go-back");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);

  action = g_action_map_lookup_action (G_ACTION_MAP (self), "go-forward");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
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

  object_class->constructed = gig_window_constructed;
  object_class->dispose = gig_window_dispose;
  object_class->finalize = gig_window_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/obyknovenius/Gig/ui/gig-window.ui");

  gtk_widget_class_bind_template_child (widget_class, GigWindow, stop_reload_button);
  gtk_widget_class_bind_template_child (widget_class, GigWindow, url_entry);
  gtk_widget_class_bind_template_child (widget_class, GigWindow, tab_view);

  gtk_widget_class_bind_template_callback (widget_class, url_entry_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, tab_view_notify_selected_page_cb);

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
gig_window_add_tab (GigWindow *self,
                    const char *uri)
{
  WebKitWebView *web_view;
  AdwTabPage *tab_page;

  g_return_if_fail (GIG_IS_WINDOW (self));
  g_return_if_fail (ADW_IS_TAB_VIEW (self->tab_view));

  web_view = WEBKIT_WEB_VIEW (webkit_web_view_new ());
  tab_page = adw_tab_view_append (self->tab_view, GTK_WIDGET (web_view));

  g_object_bind_property (web_view, "title", tab_page, "title", G_BINDING_SYNC_CREATE);

  webkit_web_view_load_uri (web_view, uri);

  adw_tab_view_set_selected_page (self->tab_view, tab_page);
}
