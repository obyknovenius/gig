#include "gig-address-bar-private.h"

#include "gig-page.h"
#include "gig-uri-utils.h"
#include "gig-web-view.h"

struct _GigAddressBar
{
  GtkWidget parent_instance;

  GtkEntry *entry;

  gboolean focused;
  gboolean modified;

  GigWebView *web_view;

  GSignalGroup *web_view_signals;
};

G_DEFINE_FINAL_TYPE (GigAddressBar, gig_address_bar, GTK_TYPE_WIDGET)

static void update_attributes (GigAddressBar *self);

static void update_primary_icon (GigAddressBar *self);

static void update_secondary_icon (GigAddressBar *self);

static void update_progress (GigAddressBar *self);

static void entry_changed_cb (GigAddressBar *self,
                              GtkEntry *entry);

static void
set_text (GigAddressBar *self,
          const gchar *text)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));

  g_signal_handlers_block_by_func (self->entry,
                                   G_CALLBACK (entry_changed_cb),
                                   self);

  gtk_editable_set_text (GTK_EDITABLE (self->entry), text ? text : "");

  update_attributes (self);

  g_signal_handlers_unblock_by_func (self->entry,
                                     G_CALLBACK (entry_changed_cb),
                                     self);
}

static void
set_focused (GigAddressBar *self,
             gboolean focused)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));

  if (self->focused == focused)
    return;

  self->focused = focused;

  update_attributes (self);
  update_primary_icon (self);
  update_secondary_icon (self);
  update_progress (self);
}

static void
set_modified (GigAddressBar *self,
              gboolean modified)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));

  if (self->modified == modified)
    return;

  self->modified = modified;

  update_attributes (self);
  update_primary_icon (self);
}

static void
update_attributes (GigAddressBar *self)
{
  const gchar *text = NULL;

  g_assert (GIG_IS_ADDRESS_BAR (self));

  text = gtk_editable_get_text (GTK_EDITABLE (self->entry));

  if (self->focused || self->modified || text[0] == '\0')
    gtk_entry_set_attributes (self->entry, NULL);
  else
    {
      g_autoptr (PangoAttrList) attrs = pango_attr_list_new ();
      PangoAttribute *attr = NULL;
      GdkRGBA color;
      gfloat dim_opacity = 0.55;
      g_autofree gchar *domain = NULL;
      guint start_index = 0, end_index = 0;

      gtk_widget_get_color (GTK_WIDGET (self->entry), &color);

      attr = pango_attr_foreground_alpha_new (
          CLAMP (color.alpha * dim_opacity * 65535. + 0.5, 0, 65535));
      pango_attr_list_insert (attrs, attr);

      if ((domain = gig_get_base_domain (text, &start_index, &end_index)))
        {
          attr = pango_attr_foreground_alpha_new (0);
          attr->start_index = start_index;
          attr->end_index = end_index;
          pango_attr_list_insert (attrs, attr);
        }

      gtk_entry_set_attributes (self->entry, attrs);
    }
}

static void
update_primary_icon (GigAddressBar *self)
{
  const gchar *icon_name = NULL;
  const gchar *uri = NULL;
  GigConnectionSecurityLevel connection_security_level = GIG_CONNECTION_SECURITY_LEVEL_TBD;

  g_assert (GIG_IS_ADDRESS_BAR (self));

  if (self->web_view)
    {
      uri = gig_web_view_get_uri (self->web_view);
      connection_security_level = gig_web_view_get_connection_security_level (self->web_view);
    }

  if (self->focused || self->modified || !uri || uri[0] == '\0')
    icon_name = "system-search-symbolic";
  else if (connection_security_level == GIG_CONNECTION_SECURITY_LEVEL_SECURE)
    icon_name = "channel-secure-symbolic";
  else if (connection_security_level == GIG_CONNECTION_SECURITY_LEVEL_INSECURE)
    icon_name = "channel-insecure-symbolic";

  gtk_entry_set_icon_from_icon_name (self->entry, GTK_ENTRY_ICON_PRIMARY,
                                     icon_name);
}

static void
update_secondary_icon (GigAddressBar *self)
{
  const gchar *icon_name = NULL;
  const gchar *text = NULL;

  g_assert (GIG_IS_ADDRESS_BAR (self));

  text = gtk_editable_get_text (GTK_EDITABLE (self->entry));

  if (self->focused && text && text[0] != '\0')
    icon_name = "edit-clear-symbolic";

  gtk_entry_set_icon_from_icon_name (self->entry, GTK_ENTRY_ICON_SECONDARY,
                                     icon_name);
}

static void
update_progress (GigAddressBar *self)
{
  WebKitWebView *web_view = NULL;
  gdouble progress = 0.0f;

  g_assert (GIG_IS_ADDRESS_BAR (self));

  web_view = WEBKIT_WEB_VIEW (self->web_view);

  if (!self->focused && web_view && webkit_web_view_is_loading (web_view))
    progress = webkit_web_view_get_estimated_load_progress (web_view);

  gtk_entry_set_progress_fraction (self->entry, progress);
}

static void
entry_changed_cb (GigAddressBar *self,
                  GtkEntry *entry)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));

  set_modified (self, TRUE);

  update_secondary_icon (self);
}

static void
entry_focus_enter_cb (GigAddressBar *self,
                      GtkEventControllerFocus *controller)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));

  set_focused (self, TRUE);
}

static void
entry_focus_leave_cb (GigAddressBar *self,
                      GtkEventControllerFocus *controller)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));
  g_assert (GIG_IS_WEB_VIEW (self->web_view));

  set_focused (self, FALSE);
}

static void
entry_activate_cb (GigAddressBar *self,
                   GtkEntry *entry)
{
  const gchar *text;

  g_assert (GIG_IS_ADDRESS_BAR (self));
  g_assert (GTK_IS_ENTRY (entry));
  g_assert (GIG_IS_WEB_VIEW (self->web_view));

  text = gtk_editable_get_text (GTK_EDITABLE (entry));
  if (!text || text[0] == '\0')
    return;

  set_modified (self, FALSE);

  gig_web_view_load_uri (self->web_view, text);

  gtk_widget_grab_focus (GTK_WIDGET (self->web_view));
}

static void
entry_icon_release_cb (GigAddressBar *self,
                       GtkEntryIconPosition icon_pos,
                       GtkEntry *entry)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));

  if (icon_pos == GTK_ENTRY_ICON_SECONDARY)
    gtk_editable_delete_text (GTK_EDITABLE (entry), 0, -1);
}

static gboolean
web_view_decide_policy_cb (GigAddressBar *self,
                           WebKitPolicyDecision *decision,
                           WebKitPolicyDecisionType decision_type,
                           GigWebView *web_view)
{
  WebKitNavigationAction *navigation_action;

  g_assert (GIG_IS_ADDRESS_BAR (self));

  if (decision_type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION)
    return FALSE;

  navigation_action =
      webkit_navigation_policy_decision_get_navigation_action (WEBKIT_NAVIGATION_POLICY_DECISION (decision));

  if (webkit_navigation_action_is_user_gesture (navigation_action))
    gig_address_bar_reset (self);

  return FALSE;
}

static void
web_view_uri_changed_cb (GigAddressBar *self,
                         GParamSpec *pspec,
                         GigWebView *web_view)
{
  const gchar *uri;

  g_assert (GIG_IS_ADDRESS_BAR (self));
  g_assert (GIG_IS_WEB_VIEW (web_view));

  if (self->modified)
    return;

  uri = gig_web_view_get_uri (web_view);
  set_text (self, uri);

  update_primary_icon (self);
}

static void
web_view_is_loading_changed_cb (GigAddressBar *self,
                                GParamSpec *pspec,
                                GigWebView *web_view)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));

  update_progress (self);
}

static void
web_view_estimated_load_progress_changed_cb (GigAddressBar *self,
                                             GParamSpec *pspec,
                                             GigWebView *web_view)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));

  update_progress (self);
}

static void
web_view_connection_security_level_changed_cb (GigAddressBar *self,
                                               GParamSpec *pspec,
                                               GigWebView *web_view)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));

  update_primary_icon (self);
}

static void
gig_address_bar_dispose (GObject *object)
{
  GigAddressBar *self = GIG_ADDRESS_BAR (object);

  g_clear_object (&self->web_view);

  g_signal_group_set_target (self->web_view_signals, NULL);

  gtk_widget_dispose_template (GTK_WIDGET (self), GIG_TYPE_ADDRESS_BAR);

  G_OBJECT_CLASS (gig_address_bar_parent_class)->dispose (object);
}

static void
gig_address_bar_finalize (GObject *object)
{
  GigAddressBar *self = GIG_ADDRESS_BAR (object);

  g_clear_object (&self->web_view_signals);

  G_OBJECT_CLASS (gig_address_bar_parent_class)->finalize (object);
}

static gboolean
gig_address_bar_grab_focus (GtkWidget *widget)
{
  GigAddressBar *self = GIG_ADDRESS_BAR (widget);

  g_assert (GIG_IS_ADDRESS_BAR (self));

  return gtk_widget_grab_focus (GTK_WIDGET (self->entry));
}

static void
gig_address_bar_css_changed (GtkWidget *widget,
                             GtkCssStyleChange *change)
{
  GigAddressBar *self = GIG_ADDRESS_BAR (widget);

  GTK_WIDGET_CLASS (gig_address_bar_parent_class)->css_changed (widget, change);

  g_assert (GIG_IS_ADDRESS_BAR (self));

  update_attributes (self);
}

static void
gig_address_bar_class_init (GigAddressBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = gig_address_bar_dispose;
  object_class->finalize = gig_address_bar_finalize;
  widget_class->grab_focus = gig_address_bar_grab_focus;
  widget_class->css_changed = gig_address_bar_css_changed;

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/obyknovenius/Gig/ui/gig-address-bar.ui");

  gtk_widget_class_bind_template_child (widget_class, GigAddressBar, entry);

  gtk_widget_class_bind_template_callback (widget_class, entry_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, entry_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, entry_icon_release_cb);
  gtk_widget_class_bind_template_callback (widget_class, entry_focus_enter_cb);
  gtk_widget_class_bind_template_callback (widget_class, entry_focus_leave_cb);
}

static void
gig_address_bar_init (GigAddressBar *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->web_view_signals = g_signal_group_new (GIG_TYPE_WEB_VIEW);

  g_signal_group_connect_object (self->web_view_signals,
                                 "decide-policy",
                                 G_CALLBACK (web_view_decide_policy_cb),
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
                                 "notify::estimated-load-progress",
                                 G_CALLBACK (web_view_estimated_load_progress_changed_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

  g_signal_group_connect_object (self->web_view_signals,
                                 "notify::connection-security-level",
                                 G_CALLBACK (web_view_connection_security_level_changed_cb),
                                 self,
                                 G_CONNECT_SWAPPED);
}

GtkWidget *
gig_address_bar_new (void)
{
  return g_object_new (GIG_TYPE_ADDRESS_BAR, NULL);
}

void
gig_address_bar_set_web_view (GigAddressBar *self,
                              GigWebView *web_view)
{
  g_return_if_fail (GIG_IS_ADDRESS_BAR (self));
  g_return_if_fail (!web_view || GIG_IS_WEB_VIEW (web_view));

  gtk_widget_set_sensitive (GTK_WIDGET (self->entry), web_view != NULL);

  g_set_object (&self->web_view, web_view);

  g_signal_group_set_target (self->web_view_signals, web_view);

  gig_address_bar_reset (self);

  update_primary_icon (self);
  update_progress (self);
}

void
gig_address_bar_reset (GigAddressBar *self)
{
  const gchar *uri = NULL;

  g_return_if_fail (GIG_IS_ADDRESS_BAR (self));

  set_modified (self, FALSE);

  if (self->web_view)
    uri = gig_web_view_get_uri (self->web_view);

  set_text (self, uri);
}
