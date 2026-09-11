#include "gig-address-bar-private.h"

#include "gig-page.h"
#include "gig-uri-utils.h"
#include "gig-web-view.h"

struct _GigAddressBar
{
  GtkWidget parent_instance;

  GtkEntry *entry;

  gboolean focused;
  gboolean editing;

  GigWebView *web_view;

  GSignalGroup *web_view_signals;
};

G_DEFINE_FINAL_TYPE (GigAddressBar, gig_address_bar, GTK_TYPE_WIDGET)

enum
{
  PROP_0,
  PROP_PRIMARY_ICON_NAME,
  PROP_PROGRESS_FRACTION,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void gig_address_bar_set_text (GigAddressBar *self,
                                      const gchar *text);

static void gig_address_bar_set_editing (GigAddressBar *self,
                                         gboolean editing);

static void gig_address_bar_set_focused (GigAddressBar *self,
                                         gboolean focused);

static const gchar *gig_address_bar_get_primary_icon_name (GigAddressBar *self);

static gdouble gig_address_bar_get_progress_fraction (GigAddressBar *self);

static void
update_attributes (GigAddressBar *self)
{
  g_autoptr (PangoAttrList) attrs = NULL;
  const gchar *text = NULL;
  g_autofree gchar *base_domain = NULL;
  guint start_index = 0, end_index = 0;
  const GdkRGBA text_color = { 0.5, 0.5, 0.5, 1.0 };
  PangoAttribute *text_attr = NULL;
  PangoAttribute *base_domain_attr = NULL;

  g_assert (GIG_IS_ADDRESS_BAR (self));

  text = gtk_editable_get_text (GTK_EDITABLE (self->entry));

  if (self->focused || self->editing || text[0] == '\0')
    {
      gtk_entry_set_attributes (GTK_ENTRY (self->entry), NULL);
      return;
    }

  attrs = pango_attr_list_new ();

  text_attr = pango_attr_foreground_new ((guint16) (text_color.red * G_MAXUINT16),
                                         (guint16) (text_color.green * G_MAXUINT16),
                                         (guint16) (text_color.blue * G_MAXUINT16));
  pango_attr_list_insert (attrs, text_attr);

  if ((base_domain = gig_get_base_domain (text,
                                          &start_index,
                                          &end_index)))
    {
      base_domain_attr = pango_attr_foreground_new (0, 0, 0);
      base_domain_attr->start_index = start_index;
      base_domain_attr->end_index = end_index;
      pango_attr_list_insert (attrs, base_domain_attr);
    }

  gtk_entry_set_attributes (GTK_ENTRY (self->entry), attrs);
}

static void entry_changed_cb (GigAddressBar *self,
                              GtkEntry *entry);

static void
entry_changed_cb (GigAddressBar *self,
                  GtkEntry *entry)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));

  gig_address_bar_set_editing (self, TRUE);
}

static void
entry_focus_enter_cb (GigAddressBar *self,
                      GtkEventControllerFocus *controller)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));

  gig_address_bar_set_focused (self, TRUE);
}

static void
entry_focus_leave_cb (GigAddressBar *self,
                      GtkEventControllerFocus *controller)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));
  g_assert (GIG_IS_WEB_VIEW (self->web_view));

  gig_address_bar_set_focused (self, FALSE);
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

  gig_address_bar_set_editing (self, FALSE);

  gig_web_view_load_address (self->web_view, text);

  gtk_widget_grab_focus (GTK_WIDGET (self->web_view));
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
web_view_address_changed_cb (GigAddressBar *self,
                             GParamSpec *pspec,
                             GigWebView *web_view)
{
  const gchar *address;

  g_assert (GIG_IS_ADDRESS_BAR (self));
  g_assert (GIG_IS_WEB_VIEW (web_view));

  if (self->editing)
    return;

  address = gig_web_view_get_address (web_view);
  gig_address_bar_set_text (self, address);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PRIMARY_ICON_NAME]);
}

static void
web_view_is_loading_changed_cb (GigAddressBar *self,
                                GParamSpec *pspec,
                                GigWebView *web_view)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PROGRESS_FRACTION]);
}

static void
web_view_estimated_load_progress_changed_cb (GigAddressBar *self,
                                             GParamSpec *pspec,
                                             GigWebView *web_view)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PROGRESS_FRACTION]);
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

static void
gig_address_bar_get_property (GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
  GigAddressBar *self = GIG_ADDRESS_BAR (object);

  switch (prop_id)
    {
    case PROP_PRIMARY_ICON_NAME:
      g_value_set_string (value, gig_address_bar_get_primary_icon_name (self));
      break;

    case PROP_PROGRESS_FRACTION:
      g_value_set_double (value, gig_address_bar_get_progress_fraction (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gboolean
gig_address_bar_grab_focus (GtkWidget *widget)
{
  GigAddressBar *self = GIG_ADDRESS_BAR (widget);

  g_assert (GIG_IS_ADDRESS_BAR (self));

  return gtk_widget_grab_focus (GTK_WIDGET (self->entry));
}

static void
gig_address_bar_class_init (GigAddressBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = gig_address_bar_dispose;
  object_class->finalize = gig_address_bar_finalize;
  object_class->get_property = gig_address_bar_get_property;
  widget_class->grab_focus = gig_address_bar_grab_focus;

  properties[PROP_PRIMARY_ICON_NAME] =
      g_param_spec_string ("primary-icon-name", NULL, NULL,
                           NULL,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_PROGRESS_FRACTION] =
      g_param_spec_double ("progress-fraction", NULL, NULL,
                           0.0, 1.0, 0.0,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/obyknovenius/Gig/ui/gig-address-bar.ui");

  gtk_widget_class_bind_template_child (widget_class, GigAddressBar, entry);

  gtk_widget_class_bind_template_callback (widget_class, entry_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, entry_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, entry_focus_enter_cb);
  gtk_widget_class_bind_template_callback (widget_class, entry_focus_leave_cb);
}

static void
gig_address_bar_init (GigAddressBar *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  g_object_bind_property (self, "primary-icon-name",
                          self->entry, "primary-icon-name",
                          G_BINDING_SYNC_CREATE);

  g_object_bind_property (self, "progress-fraction",
                          self->entry, "progress-fraction",
                          G_BINDING_SYNC_CREATE);

  self->web_view_signals = g_signal_group_new (GIG_TYPE_WEB_VIEW);

  g_signal_group_connect_object (self->web_view_signals,
                                 "decide-policy",
                                 G_CALLBACK (web_view_decide_policy_cb),
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
                                 "notify::estimated-load-progress",
                                 G_CALLBACK (web_view_estimated_load_progress_changed_cb),
                                 self,
                                 G_CONNECT_SWAPPED);
}

GtkWidget *
gig_address_bar_new (void)
{
  return g_object_new (GIG_TYPE_ADDRESS_BAR, NULL);
}

static void
gig_address_bar_set_text (GigAddressBar *self,
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
gig_address_bar_set_focused (GigAddressBar *self,
                             gboolean focused)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));

  if (self->focused == focused)
    return;

  self->focused = focused;

  update_attributes (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PROGRESS_FRACTION]);
}

static void
gig_address_bar_set_editing (GigAddressBar *self,
                             gboolean editing)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));

  if (self->editing == editing)
    return;

  self->editing = editing;

  update_attributes (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PRIMARY_ICON_NAME]);
}

static const gchar *
gig_address_bar_get_primary_icon_name (GigAddressBar *self)
{
  const gchar *address = NULL;

  g_assert (GIG_IS_ADDRESS_BAR (self));

  if (self->editing)
    return "system-search-symbolic";

  if (self->web_view)
    address = gig_web_view_get_address (self->web_view);

  if (!address || address[0] == '\0')
    return "system-search-symbolic";

  return NULL;
}

static gdouble
gig_address_bar_get_progress_fraction (GigAddressBar *self)
{
  g_assert (GIG_IS_ADDRESS_BAR (self));
  g_assert (GTK_IS_ENTRY (self->entry));

  if (self->focused)
    return 0.0;

  if (self->web_view && webkit_web_view_is_loading (WEBKIT_WEB_VIEW (self->web_view)))
    return webkit_web_view_get_estimated_load_progress (WEBKIT_WEB_VIEW (self->web_view));

  return 0.0;
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

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PRIMARY_ICON_NAME]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PROGRESS_FRACTION]);
}

void
gig_address_bar_reset (GigAddressBar *self)
{
  const gchar *address = NULL;

  g_return_if_fail (GIG_IS_ADDRESS_BAR (self));

  gig_address_bar_set_editing (self, FALSE);

  if (self->web_view)
    address = gig_web_view_get_address (self->web_view);

  gig_address_bar_set_text (self, address);
}
