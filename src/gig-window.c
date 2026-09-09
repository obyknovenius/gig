#include "gig-window-private.h"

#include "gig-page.h"
#include "gig-url-entry.h"
#include "gig-web-view.h"

G_DEFINE_TYPE (GigWindow, gig_window, ADW_TYPE_APPLICATION_WINDOW)

static AdwTabPage *
tab_overview_create_tab_cb (GigWindow *self,
                            AdwTabOverview *tab_overview)
{
  GigWebView *web_view = NULL;
  GigPage *page = NULL;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (ADW_IS_TAB_OVERVIEW (tab_overview));

  web_view = GIG_WEB_VIEW (gig_web_view_new ());
  page = gig_page_new (web_view);

  gig_window_add_tab_page (self, page, FALSE, NULL);

  return adw_tab_view_get_page (self->tab_view, GTK_WIDGET (page));
}

static WebKitWebView *
web_view_create_cb (GigWindow *self,
                    WebKitNavigationAction *navigation_action,
                    WebKitWebView *related_web_view)
{
  GigWebView *web_view = NULL;
  GigPage *page = NULL;
  WebKitURIRequest *request = NULL;
  const gchar *pending_uri = NULL;
  AdwTabPage *parent = NULL;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (related_web_view));

  web_view = GIG_WEB_VIEW (gig_web_view_new_with_related_view (related_web_view));

  request = webkit_navigation_action_get_request (navigation_action);
  pending_uri = webkit_uri_request_get_uri (request);
  gig_web_view_set_pending_address (web_view, pending_uri);

  page = gig_page_new (web_view);
  parent = adw_tab_view_get_page (self->tab_view, GTK_WIDGET (self->selected_page));

  gig_window_add_tab_page (self, page, TRUE, parent);

  return WEBKIT_WEB_VIEW (web_view);
}

static void
web_view_address_changed_cb (GigWindow *self,
                             GParamSpec *pspec,
                             GigWebView *web_view)
{
  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_WEB_VIEW (web_view));

  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "win.stop-reload",
                                 !gig_web_view_is_blank (web_view));
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

static void
tab_view_selected_page_changed_cb (GigWindow *self,
                                   GParamSpec *pspec,
                                   AdwTabView *tab_view)
{
  AdwTabPage *tab_page = NULL;
  GigPage *page = NULL;
  GigWebView *web_view = NULL;
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
      is_loading = webkit_web_view_is_loading (WEBKIT_WEB_VIEW (web_view));
    }

  gtk_button_set_icon_name (GTK_BUTTON (self->stop_reload_button),
                            is_loading ? "process-stop-symbolic"
                                       : "view-refresh-symbolic");

  gig_url_entry_set_web_view (GIG_URL_ENTRY (self->url_entry), web_view);

  gig_window_update_actions (self, web_view);

  g_signal_group_set_target (self->web_view_signals, web_view);

  self->selected_page = page;

  if (web_view && !gig_web_view_is_blank (web_view))
    gtk_widget_grab_focus (GTK_WIDGET (page));
  else
    gtk_widget_grab_focus (GTK_WIDGET (self->url_entry));
}

static void
gig_window_dispose (GObject *object)
{
  GigWindow *self = GIG_WINDOW (object);

  g_assert (GIG_IS_WINDOW (self));

  g_signal_group_set_target (self->web_view_signals, NULL);

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

  self->web_view_signals = g_signal_group_new (GIG_TYPE_WEB_VIEW);

  g_signal_group_connect_object (self->web_view_signals,
                                 "create",
                                 G_CALLBACK (web_view_create_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

  g_signal_group_connect_object (self->web_view_signals,
                                 "notify::address",
                                 G_CALLBACK (web_view_address_changed_cb),
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
gig_window_add_tab_page (GigWindow *self,
                         GigPage *page,
                         gboolean set_selected,
                         AdwTabPage *parent)
{
  AdwTabPage *tab_page;

  g_return_if_fail (GIG_IS_WINDOW (self));
  g_return_if_fail (GIG_IS_PAGE (page));
  g_return_if_fail (!parent || ADW_IS_TAB_PAGE (parent));

  tab_page = adw_tab_view_add_page (self->tab_view,
                                    GTK_WIDGET (page),
                                    parent);

  g_object_bind_property (page, "title",
                          tab_page, "title",
                          G_BINDING_SYNC_CREATE);

  g_object_bind_property (page, "icon",
                          tab_page, "icon",
                          G_BINDING_SYNC_CREATE);

  g_object_bind_property (page, "is-loading",
                          tab_page, "loading",
                          G_BINDING_SYNC_CREATE);

  if (set_selected)
    adw_tab_view_set_selected_page (self->tab_view, tab_page);
}
