#include "gig-url-entry-private.h"

#include "gig-page.h"
#include "gig-utils.h"
#include "gig-web-view.h"

struct _GigUrlEntry
{
  GtkWidget parent_instance;

  GtkEntry *entry;

  gboolean focused;
  gboolean editing;

  PangoAttrList *attributes;

  GigWebView *web_view;

  GSignalGroup *web_view_signals;
};

G_DEFINE_FINAL_TYPE (GigUrlEntry, gig_url_entry, GTK_TYPE_WIDGET)

enum
{
  PROP_0,
  PROP_PRIMARY_ICON_NAME,
  PROP_PROGRESS_FRACTION,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void gig_url_entry_set_text (GigUrlEntry *self,
                                    const gchar *text);

static void gig_url_entry_set_editing (GigUrlEntry *self,
                                       gboolean editing);

static void gig_url_entry_set_focused (GigUrlEntry *self,
                                       gboolean focused);

static const gchar *gig_url_entry_get_primary_icon_name (GigUrlEntry *self);

static gdouble gig_url_entry_get_progress_fraction (GigUrlEntry *self);

static void
update_attributes (GigUrlEntry *self,
                   const gchar *address)
{
  g_autoptr (PangoAttrList) attributes = NULL;
  g_autofree gchar *base_domain = NULL;
  PangoAttribute *normal_color = NULL;
  PangoAttribute *dimmed_color = NULL;
  const gchar *substring = NULL;

  g_assert (GIG_IS_URL_ENTRY (self));

  if (!address)
    {
      g_clear_pointer (&self->attributes, pango_attr_list_unref);
      return;
    }

  attributes = pango_attr_list_new ();

  dimmed_color = pango_attr_foreground_alpha_new (32768);
  pango_attr_list_insert (attributes, dimmed_color);

  base_domain = gig_utils_get_base_domain (address);
  if (base_domain)
    substring = strstr (address, base_domain);

  if (substring)
    {
      normal_color = pango_attr_foreground_alpha_new (65535);
      normal_color->start_index = substring - address;
      normal_color->end_index = normal_color->start_index + strlen (base_domain);
      pango_attr_list_insert (attributes, normal_color);
    }

  g_clear_pointer (&self->attributes, pango_attr_list_unref);
  self->attributes = g_steal_pointer (&attributes);
}

static void entry_changed_cb (GigUrlEntry *self,
                              GtkEntry *entry);

static void
entry_changed_cb (GigUrlEntry *self,
                  GtkEntry *entry)
{
  g_assert (GIG_IS_URL_ENTRY (self));

  gig_url_entry_set_editing (self, TRUE);
}

static void
entry_focus_enter_cb (GigUrlEntry *self,
                      GtkEventControllerFocus *controller)
{
  g_assert (GIG_IS_URL_ENTRY (self));

  gig_url_entry_set_focused (self, TRUE);
  gtk_entry_set_attributes (GTK_ENTRY (self->entry), NULL);
}

static void
entry_focus_leave_cb (GigUrlEntry *self,
                      GtkEventControllerFocus *controller)
{
  g_assert (GIG_IS_URL_ENTRY (self));
  g_assert (GIG_IS_WEB_VIEW (self->web_view));

  gig_url_entry_set_focused (self, FALSE);
  gtk_entry_set_attributes (GTK_ENTRY (self->entry), self->attributes);
}

static void
entry_activate_cb (GigUrlEntry *self,
                   GtkEntry *entry)
{
  const gchar *text;

  g_assert (GIG_IS_URL_ENTRY (self));
  g_assert (GTK_IS_ENTRY (entry));
  g_assert (GIG_IS_WEB_VIEW (self->web_view));

  text = gtk_editable_get_text (GTK_EDITABLE (entry));
  if (!text || text[0] == '\0')
    return;

  gig_url_entry_set_editing (self, FALSE);

  gig_web_view_load_address (self->web_view, text);

  gtk_widget_grab_focus (GTK_WIDGET (self->web_view));
}

static gboolean
web_view_decide_policy_cb (GigUrlEntry *self,
                           WebKitPolicyDecision *decision,
                           WebKitPolicyDecisionType decision_type,
                           GigWebView *web_view)
{
  WebKitNavigationAction *navigation_action;

  g_assert (GIG_IS_URL_ENTRY (self));

  if (decision_type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION)
    return FALSE;

  navigation_action =
      webkit_navigation_policy_decision_get_navigation_action (WEBKIT_NAVIGATION_POLICY_DECISION (decision));

  if (webkit_navigation_action_is_user_gesture (navigation_action))
    gig_url_entry_reset (self);

  return FALSE;
}

static void
web_view_address_changed_cb (GigUrlEntry *self,
                             GParamSpec *pspec,
                             GigWebView *web_view)
{
  const gchar *address;

  g_assert (GIG_IS_URL_ENTRY (self));
  g_assert (GIG_IS_WEB_VIEW (web_view));

  address = gig_web_view_get_address (web_view);
  update_attributes (self, address);

  if (self->editing)
    return;

  gig_url_entry_set_text (self, address);
  gtk_entry_set_attributes (GTK_ENTRY (self->entry), self->attributes);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PRIMARY_ICON_NAME]);
}

static void
web_view_is_loading_changed_cb (GigUrlEntry *self,
                                GParamSpec *pspec,
                                GigWebView *web_view)
{
  g_assert (GIG_IS_URL_ENTRY (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PROGRESS_FRACTION]);
}

static void
web_view_estimated_load_progress_changed_cb (GigUrlEntry *self,
                                             GParamSpec *pspec,
                                             GigWebView *web_view)
{
  g_assert (GIG_IS_URL_ENTRY (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PROGRESS_FRACTION]);
}

static void
gig_url_entry_dispose (GObject *object)
{
  GigUrlEntry *self = GIG_URL_ENTRY (object);

  g_clear_object (&self->web_view);

  g_signal_group_set_target (self->web_view_signals, NULL);

  gtk_widget_dispose_template (GTK_WIDGET (self), GIG_TYPE_URL_ENTRY);

  G_OBJECT_CLASS (gig_url_entry_parent_class)->dispose (object);
}

static void
gig_url_entry_finalize (GObject *object)
{
  GigUrlEntry *self = GIG_URL_ENTRY (object);

  g_clear_object (&self->web_view_signals);
  g_clear_pointer (&self->attributes, pango_attr_list_unref);

  G_OBJECT_CLASS (gig_url_entry_parent_class)->finalize (object);
}

static void
gig_url_entry_get_property (GObject *object,
                            guint prop_id,
                            GValue *value,
                            GParamSpec *pspec)
{
  GigUrlEntry *self = GIG_URL_ENTRY (object);

  switch (prop_id)
    {
    case PROP_PRIMARY_ICON_NAME:
      g_value_set_string (value, gig_url_entry_get_primary_icon_name (self));
      break;

    case PROP_PROGRESS_FRACTION:
      g_value_set_double (value, gig_url_entry_get_progress_fraction (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gboolean
gig_url_entry_grab_focus (GtkWidget *widget)
{
  GigUrlEntry *self = GIG_URL_ENTRY (widget);

  g_assert (GIG_IS_URL_ENTRY (self));

  return gtk_widget_grab_focus (GTK_WIDGET (self->entry));
}

static void
gig_url_entry_class_init (GigUrlEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = gig_url_entry_dispose;
  object_class->finalize = gig_url_entry_finalize;
  object_class->get_property = gig_url_entry_get_property;
  widget_class->grab_focus = gig_url_entry_grab_focus;

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

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/obyknovenius/Gig/ui/gig-url-entry.ui");

  gtk_widget_class_bind_template_child (widget_class, GigUrlEntry, entry);

  gtk_widget_class_bind_template_callback (widget_class, entry_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, entry_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, entry_focus_enter_cb);
  gtk_widget_class_bind_template_callback (widget_class, entry_focus_leave_cb);
}

static void
gig_url_entry_init (GigUrlEntry *self)
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

GigUrlEntry *
gig_url_entry_new (void)
{
  return g_object_new (GIG_TYPE_URL_ENTRY, NULL);
}

static void
gig_url_entry_set_text (GigUrlEntry *self,
                        const gchar *text)
{
  g_assert (GIG_IS_URL_ENTRY (self));

  g_signal_handlers_block_by_func (self->entry,
                                   G_CALLBACK (entry_changed_cb),
                                   self);

  gtk_editable_set_text (GTK_EDITABLE (self->entry), text ? text : "");

  g_signal_handlers_unblock_by_func (self->entry,
                                     G_CALLBACK (entry_changed_cb),
                                     self);
}

static void
gig_url_entry_set_focused (GigUrlEntry *self,
                           gboolean focused)
{
  g_assert (GIG_IS_URL_ENTRY (self));

  if (self->focused == focused)
    return;

  self->focused = focused;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PROGRESS_FRACTION]);
}

static void
gig_url_entry_set_editing (GigUrlEntry *self,
                           gboolean editing)
{
  g_assert (GIG_IS_URL_ENTRY (self));

  if (self->editing == editing)
    return;

  self->editing = editing;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PRIMARY_ICON_NAME]);
}

static const gchar *
gig_url_entry_get_primary_icon_name (GigUrlEntry *self)
{
  const gchar *address = NULL;

  g_assert (GIG_IS_URL_ENTRY (self));

  if (self->editing)
    return "system-search-symbolic";

  if (self->web_view)
    address = gig_web_view_get_address (self->web_view);

  if (!address || address[0] == '\0')
    return "system-search-symbolic";

  return NULL;
}

static gdouble
gig_url_entry_get_progress_fraction (GigUrlEntry *self)
{
  g_assert (GIG_IS_URL_ENTRY (self));
  g_assert (GTK_IS_ENTRY (self->entry));

  if (self->focused)
    return 0.0;

  if (self->web_view && webkit_web_view_is_loading (WEBKIT_WEB_VIEW (self->web_view)))
    return webkit_web_view_get_estimated_load_progress (WEBKIT_WEB_VIEW (self->web_view));

  return 0.0;
}

void
gig_url_entry_set_web_view (GigUrlEntry *self,
                            GigWebView *web_view)
{
  g_return_if_fail (GIG_IS_URL_ENTRY (self));
  g_return_if_fail (!web_view || GIG_IS_WEB_VIEW (web_view));

  gtk_widget_set_sensitive (GTK_WIDGET (self->entry), web_view != NULL);

  g_set_object (&self->web_view, web_view);

  g_signal_group_set_target (self->web_view_signals, web_view);

  gig_url_entry_reset (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PRIMARY_ICON_NAME]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PROGRESS_FRACTION]);
}

void
gig_url_entry_reset (GigUrlEntry *self)
{
  const gchar *address = NULL;

  g_return_if_fail (GIG_IS_URL_ENTRY (self));

  if (self->web_view)
    address = gig_web_view_get_address (self->web_view);

  gig_url_entry_set_text (self, address);
  gig_url_entry_set_editing (self, FALSE);

  update_attributes (self, address);
  gtk_entry_set_attributes (GTK_ENTRY (self->entry), self->attributes);
}
