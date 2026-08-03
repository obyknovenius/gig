#include "gig-window.h"

struct _GigWindow
{
  AdwApplicationWindow parent_instance;

  GtkButton *stop_reload_button;
  GtkEntry *url_entry;
  WebKitWebView *web_view;
};

G_DEFINE_TYPE (GigWindow, gig_window, ADW_TYPE_APPLICATION_WINDOW)

static void
gig_window_go_back (GSimpleAction *action,
                    GVariant *parameter,
                    gpointer user_data)
{
  GigWindow *self = GIG_WINDOW (user_data);

  g_assert (GIG_IS_WINDOW (self));

  webkit_web_view_go_back (self->web_view);
}

static void
gig_window_go_forward (GSimpleAction *action,
                       GVariant *parameter,
                       gpointer user_data)
{
  GigWindow *self = GIG_WINDOW (user_data);

  g_assert (GIG_IS_WINDOW (self));

  webkit_web_view_go_forward (self->web_view);
}

static void
gig_window_stop_reload (GSimpleAction *action,
                        GVariant *parameter,
                        gpointer user_data)
{
  GigWindow *self = GIG_WINDOW (user_data);
  g_autoptr (GVariant) state = NULL;

  g_assert (GIG_IS_WINDOW (self));

  state = g_action_get_state (G_ACTION (action));
  if (g_variant_get_boolean (state))
    webkit_web_view_stop_loading (self->web_view);
  else
    webkit_web_view_reload (self->web_view);
}

static const GActionEntry actions[] = {
  { "go-back", gig_window_go_back },
  { "go-forward", gig_window_go_forward },
  { "stop-reload", gig_window_stop_reload, NULL, "false" },
};

static void
on_url_entry_activated (GigWindow *self,
                        GtkEntry *entry)
{
  const char *uri;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (GTK_IS_ENTRY (entry));

  uri = gtk_editable_get_text (GTK_EDITABLE (entry));
  webkit_web_view_load_uri (self->web_view, uri);
}

static void
on_web_view_uri_changed (GigWindow *self,
                         GParamSpec *pspec,
                         WebKitWebView *web_view)
{
  const char *uri;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  uri = webkit_web_view_get_uri (web_view);
  gtk_editable_set_text (GTK_EDITABLE (self->url_entry), uri);
}

static void
on_back_forward_list_changed (GigWindow *self)
{
  GAction *action;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (self->web_view));

  action = g_action_map_lookup_action (G_ACTION_MAP (self), "go-back");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
                               webkit_web_view_can_go_back (self->web_view));

  action = g_action_map_lookup_action (G_ACTION_MAP (self), "go-forward");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
                               webkit_web_view_can_go_forward (self->web_view));
}

static void
on_web_view_is_loading_changed (GigWindow *self,
                                GParamSpec *pspec,
                                WebKitWebView *web_view)
{
  gboolean is_loading;
  GAction *action;

  g_assert (GIG_IS_WINDOW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  is_loading = webkit_web_view_is_loading (web_view);

  action = g_action_map_lookup_action (G_ACTION_MAP (self), "stop-reload");
  g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (is_loading));

  gtk_button_set_icon_name (GTK_BUTTON (self->stop_reload_button),
                            is_loading ? "process-stop-symbolic"
                                       : "view-refresh-symbolic");
}

static void
gig_window_constructed (GObject *object)
{
  GigWindow *self = GIG_WINDOW (object);
  WebKitBackForwardList *back_forward_list;
  GAction *action;

  g_assert (GIG_IS_WINDOW (self));

  G_OBJECT_CLASS (gig_window_parent_class)->constructed (object);

  g_action_map_add_action_entries (G_ACTION_MAP (self), actions,
                                   G_N_ELEMENTS (actions), self);

  action = g_action_map_lookup_action (G_ACTION_MAP (self), "go-back");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);

  action = g_action_map_lookup_action (G_ACTION_MAP (self), "go-forward");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);

  // FIXME: Bind "can-go-back" and "can-go-forward" properties when implemented.
  back_forward_list = webkit_web_view_get_back_forward_list (self->web_view);
  g_signal_connect_object (back_forward_list, "changed",
                           G_CALLBACK (on_back_forward_list_changed),
                           self, G_CONNECT_SWAPPED);
}

static void
gig_window_class_init (GigWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = gig_window_constructed;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/obyknovenius/Gig/ui/gig-window.ui");

  gtk_widget_class_bind_template_child (widget_class, GigWindow, stop_reload_button);
  gtk_widget_class_bind_template_child (widget_class, GigWindow, url_entry);
  gtk_widget_class_bind_template_child (widget_class, GigWindow, web_view);

  gtk_widget_class_bind_template_callback (widget_class, on_url_entry_activated);
  gtk_widget_class_bind_template_callback (widget_class, on_web_view_uri_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_web_view_is_loading_changed);

  g_type_ensure (WEBKIT_TYPE_WEB_VIEW);
}

static void
gig_window_init (GigWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GigWindow *
gig_window_new (GtkApplication *application)
{
  g_return_val_if_fail (G_APPLICATION (application), NULL);

  return g_object_new (GIG_TYPE_WINDOW,
                       "application", application,
                       NULL);
}

WebKitWebView *
gig_window_get_web_view (GigWindow *self)
{
  g_return_val_if_fail (GIG_IS_WINDOW (self), NULL);

  return self->web_view;
}
