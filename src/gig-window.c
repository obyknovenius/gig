#include "config.h"

#include "gig-window-private.h"

#include "gig-address-bar.h"
#include "gig-application.h"
#include "gig-settings.h"
#include "gig-tab.h"
#include "gig-web-view.h"

G_DEFINE_TYPE (GigWindow, gig_window, ADW_TYPE_APPLICATION_WINDOW)

static void
capture_state (GigWindow *self,
               gint *width,
               gint *height,
               gboolean *maximized)
{
  g_assert (GIG_IS_WINDOW (self));

  gtk_window_get_default_size (GTK_WINDOW (self), width, height);
  *maximized = gtk_window_is_maximized (GTK_WINDOW (self));
}

static void
apply_state (GigWindow *self,
             gint width,
             gint height,
             gboolean maximized)
{
  g_assert (GIG_IS_WINDOW (self));

  gtk_window_set_default_size (GTK_WINDOW (self), width, height);

  if (maximized)
    gtk_window_maximize (GTK_WINDOW (self));
}

static void
save_state (GigWindow *self)
{
  GigApplication *application;
  GigSettings *settings;
  gint width, height;
  gboolean maximized;

  g_assert (GIG_IS_WINDOW (self));

  application = GIG_APPLICATION (gtk_window_get_application (GTK_WINDOW (self)));
  settings = gig_application_get_settings (application);

  capture_state (self, &width, &height, &maximized);

  gig_settings_set_window_state (settings, width, height, maximized);
}

static void
setup_tab (GigWindow *self,
           GigTab *tab,
           gboolean set_selected)
{
  AdwTabPage *tab_page;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_TAB (tab));

  tab_page = adw_tab_view_get_page (self->tab_view, GTK_WIDGET (tab));
  g_assert (ADW_IS_TAB_PAGE (tab_page));

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

static AdwTabPage *
tab_overview_create_tab_cb (GigWindow *self,
                            AdwTabOverview *tab_overview)
{
  GigTab *tab = NULL;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (ADW_IS_TAB_OVERVIEW (tab_overview));

  tab = gig_tab_new ();

  gig_window_add_tab (self, tab, NULL, FALSE);

  return adw_tab_view_get_page (self->tab_view, GTK_WIDGET (tab));
}

static void
web_view_open_in_new_tab_cb (GigWindow *self,
                             const gchar *uri,
                             GigWebView *web_view)
{
  GigWebView *new_web_view;
  GigTab *new_tab, *parent_tab;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_WEB_VIEW (web_view));

  new_web_view = gig_web_view_new ();
  gig_web_view_load_uri (new_web_view, uri);

  new_tab = gig_tab_new_with_web_view (new_web_view);
  parent_tab = gig_window_get_selected_tab (self);
  g_assert (GIG_IS_TAB (parent_tab));

  gig_window_add_tab (self, new_tab, parent_tab, FALSE);
}

static void
web_view_open_in_new_window_cb (GigWindow *self,
                                const gchar *uri,
                                GigWebView *web_view)
{
  GigApplication *application;
  GigWindow *new_window;
  GigWebView *new_web_view;
  GigTab *new_tab;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_WEB_VIEW (web_view));

  new_web_view = gig_web_view_new ();
  gig_web_view_load_uri (new_web_view, uri);

  new_tab = gig_tab_new_with_web_view (new_web_view);

  application = GIG_APPLICATION (gtk_window_get_application (GTK_WINDOW (self)));
  new_window = gig_window_new_empty (application);

  gig_window_inherit_state (new_window, self);

  gig_window_add_tab (new_window, new_tab, NULL, FALSE);
  gtk_window_present (GTK_WINDOW (new_window));
}

static WebKitWebView *
web_view_create_cb (GigWindow *self,
                    WebKitNavigationAction *navigation_action,
                    WebKitWebView *web_view)
{
  GigWebView *new_web_view;
  GigTab *new_tab, *parent_tab;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  new_web_view = gig_web_view_new_for_navigation_action (navigation_action, web_view);
  new_tab = gig_tab_new_with_web_view (new_web_view);

  parent_tab = gig_window_get_selected_tab (self);
  g_assert (GIG_IS_TAB (parent_tab));

  gig_window_add_tab (self, new_tab, parent_tab, TRUE);

  return WEBKIT_WEB_VIEW (new_web_view);
}

static void
web_view_is_empty_changed_cb (GigWindow *self,
                              GParamSpec *pspec,
                              GigWebView *web_view)
{
  gboolean is_empty;
  gboolean is_loading;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_WEB_VIEW (web_view));

  is_empty = gig_web_view_is_empty (web_view);
  is_loading = webkit_web_view_is_loading (WEBKIT_WEB_VIEW (web_view));

  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "win.stop-reload",
                                 !is_empty || is_loading);

  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "tab.find",
                                 !is_empty);
}

static void
web_view_is_loading_changed_cb (GigWindow *self,
                                GParamSpec *pspec,
                                GigWebView *web_view)
{
  gboolean is_empty;
  gboolean is_loading;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_WEB_VIEW (web_view));

  is_empty = gig_web_view_is_empty (web_view);
  is_loading = webkit_web_view_is_loading (WEBKIT_WEB_VIEW (web_view));

  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "win.stop-reload",
                                 !is_empty || is_loading);

  gtk_button_set_icon_name (GTK_BUTTON (self->stop_reload_button),
                            is_loading
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
  gint n_pages = -1;
  gint position = -1;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (ADW_IS_TAB_VIEW (tab_view));

  if (page)
    {
      n_pages = adw_tab_view_get_n_pages (tab_view);
      position = adw_tab_view_get_page_position (tab_view, page);
    }

  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "tab.close-before",
                                 page && position > 0);

  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "tab.close-after",
                                 page && position < n_pages - 1);

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

  if (web_view)
    {
      if (!gig_web_view_get_display_uri (web_view))
        gtk_widget_grab_focus (GTK_WIDGET (self->address_bar));
      else
        gtk_widget_grab_focus (GTK_WIDGET (tab));
    }
}

static void
gig_window_dispose (GObject *object)
{
  GigWindow *self = (GigWindow *) object;

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

static gboolean
gig_window_close_request (GtkWindow *window)
{
  GigWindow *self = (GigWindow *) window;

  g_assert (GIG_IS_WINDOW (self));

  save_state (self);

  return GTK_WINDOW_CLASS (gig_window_parent_class)->close_request (window);
}

static void
gig_window_class_init (GigWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkWindowClass *window_class = GTK_WINDOW_CLASS (klass);

  object_class->dispose = gig_window_dispose;
  object_class->finalize = gig_window_finalize;

  window_class->close_request = gig_window_close_request;

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
  gtk_widget_init_template (GTK_WIDGET (self));

#if DEVELOPMENT_BUILD
  gtk_widget_add_css_class (GTK_WIDGET (self), "devel");
#endif

  gig_window_actions_init (self);

  self->web_view_signals = g_signal_group_new (GIG_TYPE_WEB_VIEW);

  g_signal_group_connect_object (self->web_view_signals,
                                 "open-in-new-tab",
                                 G_CALLBACK (web_view_open_in_new_tab_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

  g_signal_group_connect_object (self->web_view_signals,
                                 "open-in-new-window",
                                 G_CALLBACK (web_view_open_in_new_window_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

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
                                 "notify::is-empty",
                                 G_CALLBACK (web_view_is_empty_changed_cb),
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
gig_window_new (GigApplication *application)
{
  GigWindow *self;
  GigTab *tab;

  g_return_val_if_fail (GIG_IS_APPLICATION (application), NULL);

  self = gig_window_new_empty (application);
  tab = gig_tab_new ();

  gig_window_add_tab (self, tab, NULL, TRUE);

  return self;
}

GigWindow *
gig_window_new_empty (GigApplication *application)
{
  g_return_val_if_fail (GIG_IS_APPLICATION (application), NULL);

  return g_object_new (GIG_TYPE_WINDOW,
                       "application", application,
                       NULL);
}

void
gig_window_add_tab (GigWindow *self,
                    GigTab *tab,
                    GigTab *parent_tab,
                    gboolean set_selected)
{
  AdwTabPage *parent_tab_page = NULL;

  g_return_if_fail (GIG_IS_WINDOW (self));
  g_return_if_fail (GIG_IS_TAB (tab));
  g_return_if_fail (!parent_tab || GIG_IS_TAB (parent_tab));

  if (parent_tab)
    parent_tab_page = adw_tab_view_get_page (self->tab_view,
                                             GTK_WIDGET (parent_tab));

  adw_tab_view_add_page (self->tab_view, GTK_WIDGET (tab), parent_tab_page);

  setup_tab (self, tab, set_selected);
}

void
gig_window_insert_tab (GigWindow *self,
                       GigTab *tab,
                       gint position,
                       gboolean set_selected)
{
  g_return_if_fail (GIG_IS_WINDOW (self));
  g_return_if_fail (GIG_IS_TAB (tab));

  adw_tab_view_insert (self->tab_view, GTK_WIDGET (tab), position);

  setup_tab (self, tab, set_selected);
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

void
gig_window_restore_state (GigWindow *self)
{
  GigApplication *application;
  GigSettings *settings;
  gint width, height;
  gboolean maximized;

  g_return_if_fail (GIG_IS_WINDOW (self));

  application = GIG_APPLICATION (gtk_window_get_application (GTK_WINDOW (self)));
  settings = gig_application_get_settings (application);

  gig_settings_get_window_state (settings, &width, &height, &maximized);

  apply_state (self, width, height, maximized);
}

void
gig_window_inherit_state (GigWindow *self,
                          GigWindow *source)
{
  gint width, height;
  gboolean maximized;

  g_return_if_fail (GIG_IS_WINDOW (self));
  g_return_if_fail (GIG_IS_WINDOW (source));

  capture_state (source, &width, &height, &maximized);
  apply_state (self, width, height, maximized);
}
