#include "gig-window-private.h"

#include "gig-page.h"
#include "gig-utils.h"

G_DEFINE_TYPE (GigWindow, gig_window, ADW_TYPE_APPLICATION_WINDOW)

static AdwTabPage *
tab_overview_create_tab_cb (GigWindow *self,
                            AdwTabOverview *tab_overview)
{
  GigPage *page;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (ADW_IS_TAB_OVERVIEW (tab_overview));

  page = gig_page_new ();

  return gig_window_add_page (self, page);
}

static void
url_entry_changed_cb (GigWindow *self,
                      GtkEntry *entry)
{
  g_assert (GIG_IS_WINDOW (self));
  g_assert (GTK_IS_ENTRY (entry));

  gtk_entry_set_icon_from_icon_name (entry, GTK_ENTRY_ICON_PRIMARY,
                                     "system-search-symbolic");

  gtk_entry_set_icon_from_icon_name (entry, GTK_ENTRY_ICON_SECONDARY, NULL);
}

static void
url_entry_activate_cb (GigWindow *self,
                       GtkEntry *entry)
{
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

  gig_page_load_uri (self->selected_page, uri);

  gtk_widget_grab_focus (GTK_WIDGET (self->selected_page));
}

static void
page_uri_changed_cb (GigWindow *self,
                     GParamSpec *pspec,
                     GigPage *page)
{
  const gchar *uri;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_PAGE (page));

  uri = gig_page_get_uri (page);
  gtk_editable_set_text (GTK_EDITABLE (self->url_entry), uri ? uri : "");

  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "win.stop-reload",
                                 uri != NULL);

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (self->url_entry),
                                     GTK_ENTRY_ICON_PRIMARY,
                                     uri ? "info-outline-symbolic"
                                         : "system-search-symbolic");

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (self->url_entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     uri ? "user-bookmarks-symbolic" : NULL);
}

static void
page_is_loading_changed_cb (GigWindow *self,
                            GParamSpec *pspec,
                            GigPage *page)
{
  gboolean is_loading;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_PAGE (page));

  is_loading = gig_page_get_is_loading (page);

  gtk_button_set_icon_name (GTK_BUTTON (self->stop_reload_button),
                            is_loading ? "process-stop-symbolic"
                                       : "view-refresh-symbolic");

  if (!is_loading)
    gtk_entry_set_progress_fraction (self->url_entry, 0.0);
}

static void
page_estimated_load_progress_changed_cb (GigWindow *self,
                                         GParamSpec *pspec,
                                         GigPage *page)
{
  gboolean is_loading;
  gdouble progress;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_PAGE (page));

  is_loading = gig_page_get_is_loading (page);
  progress = gig_page_get_estimated_load_progress (page);

  if (is_loading)
    gtk_entry_set_progress_fraction (self->url_entry, progress);
}

static void
page_can_go_back_changed_cb (GigWindow *self,
                             GParamSpec *pspec,
                             GigPage *page)
{
  gboolean can_go_back;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_PAGE (page));

  can_go_back = gig_page_can_go_back (page);

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-back", can_go_back);
}

static void
page_can_go_forward_changed_cb (GigWindow *self,
                                GParamSpec *pspec,
                                GigPage *page)
{
  gboolean can_go_forward;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GIG_IS_PAGE (page));

  can_go_forward = gig_page_can_go_forward (page);

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "win.go-forward", can_go_forward);
}

static void
tab_view_selected_page_changed_cb (GigWindow *self,
                                   GParamSpec *pspec,
                                   AdwTabView *tab_view)
{
  GtkEntry *url_entry;
  AdwTabPage *tab_page;
  GigPage *page = NULL;
  const gchar *uri = NULL;
  gboolean is_loading = FALSE;
  gdouble progress = 0.0;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (ADW_IS_TAB_VIEW (tab_view));

  url_entry = self->url_entry;

  if ((tab_page = adw_tab_view_get_selected_page (tab_view)))
    page = GIG_PAGE (adw_tab_page_get_child (tab_page));

  if (self->selected_page == page)
    return;

  if (page)
    {
      uri = gig_page_get_uri (page);
      is_loading = gig_page_get_is_loading (page);
      progress = gig_page_get_estimated_load_progress (page);

      gtk_widget_set_sensitive (GTK_WIDGET (url_entry), TRUE);
      if (uri)
        {
          gtk_editable_set_text (GTK_EDITABLE (url_entry), uri);
          gtk_widget_grab_focus (GTK_WIDGET (page));
        }
      else
        {
          gtk_editable_set_text (GTK_EDITABLE (url_entry), "");
          gtk_widget_grab_focus (GTK_WIDGET (url_entry));
        }
    }
  else
    {
      gtk_editable_set_text (GTK_EDITABLE (url_entry), "");

      if (gtk_widget_has_focus (GTK_WIDGET (url_entry)))
        gtk_root_set_focus (GTK_ROOT (self), NULL);

      gtk_widget_set_sensitive (GTK_WIDGET (url_entry), FALSE);
    }

  gtk_entry_set_icon_from_icon_name (url_entry,
                                     GTK_ENTRY_ICON_PRIMARY,
                                     uri ? "info-outline-symbolic"
                                         : "system-search-symbolic");

  gtk_entry_set_icon_from_icon_name (url_entry,
                                     GTK_ENTRY_ICON_SECONDARY,
                                     uri ? "user-bookmarks-symbolic" : NULL);

  if (is_loading)
    gtk_entry_set_progress_fraction (self->url_entry, progress);

  gtk_button_set_icon_name (GTK_BUTTON (self->stop_reload_button),
                            is_loading ? "process-stop-symbolic"
                                       : "view-refresh-symbolic");

  gig_window_update_actions (self, page);

  g_signal_group_set_target (self->page_signals, page);

  self->selected_page = page;
}

static void
gig_window_dispose (GObject *object)
{
  GigWindow *self = GIG_WINDOW (object);

  g_assert (GIG_IS_WINDOW (self));

  g_signal_group_set_target (self->page_signals, NULL);

  G_OBJECT_CLASS (gig_window_parent_class)->dispose (object);
}

static void
gig_window_finalize (GObject *object)
{
  GigWindow *self = (GigWindow *) object;

  g_assert (GIG_IS_WINDOW (self));

  g_clear_object (&self->page_signals);

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

  gtk_widget_class_bind_template_callback (widget_class, tab_overview_create_tab_cb);
  gtk_widget_class_bind_template_callback (widget_class, url_entry_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, url_entry_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, tab_view_selected_page_changed_cb);

  gig_window_class_init_actions (klass);
}

static void
gig_window_init (GigWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->page_signals = g_signal_group_new (GIG_TYPE_PAGE);

  g_signal_group_connect_object (self->page_signals,
                                 "notify::uri",
                                 G_CALLBACK (page_uri_changed_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

  g_signal_group_connect_object (self->page_signals,
                                 "notify::is-loading",
                                 G_CALLBACK (page_is_loading_changed_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

  g_signal_group_connect_object (self->page_signals,
                                 "notify::estimated-load-progress",
                                 G_CALLBACK (page_estimated_load_progress_changed_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

  g_signal_group_connect_object (self->page_signals,
                                 "notify::can-go-back",
                                 G_CALLBACK (page_can_go_back_changed_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

  g_signal_group_connect_object (self->page_signals,
                                 "notify::can-go-forward",
                                 G_CALLBACK (page_can_go_forward_changed_cb),
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

  adw_tab_view_set_selected_page (self->tab_view, tab_page);

  return tab_page;
}
