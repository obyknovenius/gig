#include "gig-window-private.h"

#include "gig-address-bar.h"
#include "gig-tab.h"
#include "gig-web-view.h"

G_DEFINE_TYPE (GigWindow, gig_window, ADW_TYPE_APPLICATION_WINDOW)

static AdwTabPage *
tab_overview_create_tab_cb (GigWindow *self,
                            AdwTabOverview *tab_overview)
{
  GigTab *tab = NULL;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (ADW_IS_TAB_OVERVIEW (tab_overview));

  tab = GIG_TAB (gig_tab_new ());

  gig_window_add_tab (self, tab, FALSE, NULL);

  return adw_tab_view_get_page (self->tab_view, GTK_WIDGET (tab));
}

static WebKitWebView *
web_view_create_cb (GigWindow *self,
                    WebKitNavigationAction *navigation_action,
                    WebKitWebView *related_web_view)
{
  GigWebView *web_view;
  GigTab *tab;
  WebKitURIRequest *request;
  const gchar *pending_uri;
  GigTab *parent_tab;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (related_web_view));

  web_view = GIG_WEB_VIEW (gig_web_view_new_with_related_view (related_web_view));

  request = webkit_navigation_action_get_request (navigation_action);
  pending_uri = webkit_uri_request_get_uri (request);
  gig_web_view_set_pending_uri (web_view, pending_uri);

  tab = GIG_TAB (gig_tab_new_with_web_view (web_view));
  parent_tab = gig_window_get_selected_tab (self);
  g_assert (GIG_IS_TAB (parent_tab));

  gig_window_add_tab (self, tab, TRUE, parent_tab);

  return WEBKIT_WEB_VIEW (web_view);
}

static void
web_view_uri_changed_cb (GigWindow *self,
                         GParamSpec *pspec,
                         GigWebView *web_view)
{
  gboolean is_blank = TRUE;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_WEB_VIEW (web_view));

  is_blank = gig_web_view_is_blank (web_view);

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.stop-reload", !is_blank);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "tab.find", !is_blank);
}

static void
web_view_is_loading_changed_cb (GigWindow *self,
                                GParamSpec *pspec,
                                GigWebView *web_view)
{
  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_WEB_VIEW (web_view));

  gtk_button_set_icon_name (GTK_BUTTON (self->stop_reload_button),
                            webkit_web_view_is_loading (WEBKIT_WEB_VIEW (web_view))
                                ? "process-stop-symbolic"
                                : "view-refresh-symbolic");
}

static void
web_view_can_go_back_changed_cb (GigWindow *self,
                                 GParamSpec *pspec,
                                 GigWebView *web_view)
{
  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_WEB_VIEW (web_view));

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-back",
                                 webkit_web_view_can_go_back (WEBKIT_WEB_VIEW (web_view)));
}

static void
web_view_can_go_forward_changed_cb (GigWindow *self,
                                    GParamSpec *pspec,
                                    GigWebView *web_view)
{
  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_WEB_VIEW (web_view));

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-forward",
                                 webkit_web_view_can_go_forward (WEBKIT_WEB_VIEW (web_view)));
}

static gboolean
web_view_enter_fullscreen_cb (GigWindow *self,
                              GigWebView *web_view)
{
  g_assert (GIG_IS_WINDOW (self));

  gtk_widget_set_visible (GTK_WIDGET (self->header_bar), FALSE);
  gtk_widget_set_visible (GTK_WIDGET (self->tab_bar), FALSE);

  return FALSE;
}

static gboolean
web_view_leave_fullscreen_cb (GigWindow *self,
                              GigWebView *web_view)
{
  g_assert (GIG_IS_WINDOW (self));

  gtk_widget_set_visible (GTK_WIDGET (self->header_bar), TRUE);
  gtk_widget_set_visible (GTK_WIDGET (self->tab_bar), TRUE);

  return FALSE;
}

static void
tab_view_setup_menu_cb (GigWindow *self,
                        AdwTabPage *page,
                        AdwTabView *tab_view)
{
  g_assert (GIG_IS_WINDOW (self));
  g_assert (ADW_IS_TAB_VIEW (tab_view));

  self->menu_page = page;
}

static void
tab_view_selected_page_changed_cb (GigWindow *self,
                                   GParamSpec *pspec,
                                   AdwTabView *tab_view)
{
  GigTab *tab = NULL;
  GigWebView *web_view = NULL;
  gboolean is_loading = FALSE;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (ADW_IS_TAB_VIEW (tab_view));

  if ((tab = gig_window_get_selected_tab (self)))
    {
      web_view = gig_tab_get_web_view (tab);
      is_loading = webkit_web_view_is_loading (WEBKIT_WEB_VIEW (web_view));
    }

  gtk_button_set_icon_name (GTK_BUTTON (self->stop_reload_button),
                            is_loading ? "process-stop-symbolic"
                                       : "view-refresh-symbolic");

  gig_address_bar_set_web_view (GIG_ADDRESS_BAR (self->address_bar), web_view);

  gig_window_actions_update (self);

  g_signal_group_set_target (self->web_view_signals, web_view);

  if (web_view && !gig_web_view_is_blank (web_view))
    gtk_widget_grab_focus (GTK_WIDGET (tab));
  else
    gtk_widget_grab_focus (GTK_WIDGET (self->address_bar));
}

static void
gig_window_dispose (GObject *object)
{
  GigWindow *self = GIG_WINDOW (object);

  g_assert (GIG_IS_WINDOW (self));

  g_signal_group_set_target (self->web_view_signals, NULL);

  gtk_widget_dispose_template (GTK_WIDGET (self), GIG_TYPE_WINDOW);

  G_OBJECT_CLASS (gig_window_parent_class)->dispose (object);
}

static void
gig_window_finalize (GObject *object)
{
  GigWindow *self = (GigWindow *) object;

  g_assert (GIG_IS_WINDOW (self));

  g_clear_object (&self->web_view_signals);

  G_OBJECT_CLASS (gig_window_parent_class)->finalize (object);
}

static void
gig_window_class_init (GigWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = gig_window_dispose;
  object_class->finalize = gig_window_finalize;

  gig_window_class_actions_init (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/obyknovenius/Gig/gig-window.ui");

  gtk_widget_class_bind_template_child (widget_class, GigWindow, toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, GigWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, GigWindow, tab_bar);
  gtk_widget_class_bind_template_child (widget_class, GigWindow, stop_reload_button);
  gtk_widget_class_bind_template_child (widget_class, GigWindow, address_bar);
  gtk_widget_class_bind_template_child (widget_class, GigWindow, tab_view);

  gtk_widget_class_bind_template_callback (widget_class, tab_overview_create_tab_cb);
  gtk_widget_class_bind_template_callback (widget_class, tab_view_setup_menu_cb);
  gtk_widget_class_bind_template_callback (widget_class, tab_view_selected_page_changed_cb);

  g_type_ensure (GIG_TYPE_ADDRESS_BAR);
}

static void
gig_window_init (GigWindow *self)
{
  GSettings *settings = g_settings_new ("com.github.obyknovenius.Gig.State");

  g_settings_bind (settings, "width",
                   self, "default-width",
                   G_SETTINGS_BIND_DEFAULT);

  g_settings_bind (settings, "height",
                   self, "default-height",
                   G_SETTINGS_BIND_DEFAULT);

  g_settings_bind (settings, "is-maximized",
                   self, "maximized",
                   G_SETTINGS_BIND_DEFAULT);

  g_settings_bind (settings, "is-fullscreen",
                   self, "fullscreened",
                   G_SETTINGS_BIND_DEFAULT);

  gtk_widget_init_template (GTK_WIDGET (self));

  gig_window_actions_init (self);

  self->web_view_signals = g_signal_group_new (GIG_TYPE_WEB_VIEW);

  g_signal_group_connect_object (self->web_view_signals,
                                 "create",
                                 G_CALLBACK (web_view_create_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

  g_signal_group_connect_object (self->web_view_signals,
                                 "enter-fullscreen",
                                 G_CALLBACK (web_view_enter_fullscreen_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

  g_signal_group_connect_object (self->web_view_signals,
                                 "leave-fullscreen",
                                 G_CALLBACK (web_view_leave_fullscreen_cb),
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

  g_signal_group_connect_object (self->web_view_signals,
                                 "notify::can-go-back",
                                 G_CALLBACK (web_view_can_go_back_changed_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

  g_signal_group_connect_object (self->web_view_signals,
                                 "notify::can-go-forward",
                                 G_CALLBACK (web_view_can_go_forward_changed_cb),
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
                    GigTab *tab,
                    gboolean set_selected,
                    GigTab *parent_tab)
{
  AdwTabPage *tab_page;
  AdwTabPage *parent_tab_page = NULL;

  g_return_if_fail (GIG_IS_WINDOW (self));
  g_return_if_fail (GIG_IS_TAB (tab));
  g_return_if_fail (!parent_tab || GIG_IS_TAB (parent_tab));

  if (parent_tab)
    parent_tab_page = adw_tab_view_get_page (self->tab_view, GTK_WIDGET (parent_tab));

  tab_page = adw_tab_view_add_page (self->tab_view,
                                    GTK_WIDGET (tab),
                                    parent_tab_page);

  g_object_bind_property (tab, "title",
                          tab_page, "title",
                          G_BINDING_SYNC_CREATE);

  g_object_bind_property (tab, "icon",
                          tab_page, "icon",
                          G_BINDING_SYNC_CREATE);

  g_object_bind_property (tab, "is-loading",
                          tab_page, "loading",
                          G_BINDING_SYNC_CREATE);

  if (set_selected)
    adw_tab_view_set_selected_page (self->tab_view, tab_page);
}

GigTab *
gig_window_get_selected_tab (GigWindow *self)
{
  AdwTabPage *tab_page;

  g_return_val_if_fail (GIG_IS_WINDOW (self), NULL);

  tab_page = adw_tab_view_get_selected_page (self->tab_view);

  if (!tab_page)
    return NULL;

  return GIG_TAB (adw_tab_page_get_child (tab_page));
}
