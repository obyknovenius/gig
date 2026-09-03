#include "gig-url-entry.h"

#include "gig-page.h"
#include "gig-utils.h"

struct _GigUrlEntry
{
  GtkWidget parent_instance;

  GtkEntry *entry;

  gboolean is_focused;
  gboolean is_editing;
  const gchar *primary_icon_name;
  gdouble progress_fraction;

  WebKitWebView *web_view;

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

static void
entry_changed_cb (GigUrlEntry *self, GtkEntry *entry);

static void
set_text (GigUrlEntry *self,
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
update_primary_icon (GigUrlEntry *self)
{
  const gchar *uri = NULL;
  const gchar *icon_name = NULL;

  g_assert (GIG_IS_URL_ENTRY (self));

  if (self->web_view)
    uri = webkit_web_view_get_uri (self->web_view);

  if (self->is_editing || !uri || uri[0] == '\0')
    icon_name = "system-search-symbolic";

  if (g_strcmp0 (self->primary_icon_name, icon_name) == 0)
    return;

  self->primary_icon_name = icon_name;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PRIMARY_ICON_NAME]);
}

static void
update_progress_fraction (GigUrlEntry *self)
{
  gdouble progress_fraction = 0.0;
  gboolean is_loading = FALSE;

  g_assert (GIG_IS_URL_ENTRY (self));
  g_assert (GTK_IS_ENTRY (self->entry));

  if (self->web_view)
    is_loading = webkit_web_view_is_loading (self->web_view);

  if (!self->is_focused && is_loading)
    progress_fraction = webkit_web_view_get_estimated_load_progress (self->web_view);

  if (self->progress_fraction == progress_fraction)
    return;

  self->progress_fraction = progress_fraction;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PROGRESS_FRACTION]);
}

static void
set_is_focused (GigUrlEntry *self,
                gboolean is_focused)
{
  g_assert (GIG_IS_URL_ENTRY (self));

  if (self->is_focused == is_focused)
    return;

  self->is_focused = is_focused;

  update_progress_fraction (self);
}

static void
set_is_editing (GigUrlEntry *self,
                gboolean is_editing)
{
  g_assert (GIG_IS_URL_ENTRY (self));

  if (self->is_editing == is_editing)
    return;

  self->is_editing = is_editing;

  update_primary_icon (self);
}

static void
entry_changed_cb (GigUrlEntry *self,
                  GtkEntry *entry)
{
  g_assert (GIG_IS_URL_ENTRY (self));

  set_is_editing (self, TRUE);
}

static void
entry_focus_enter_cb (GigUrlEntry *self,
                      GtkEventControllerFocus *controller)
{
  g_assert (GIG_IS_URL_ENTRY (self));

  set_is_focused (self, TRUE);
}

static void
entry_focus_leave_cb (GigUrlEntry *self,
                      GtkEventControllerFocus *controller)
{
  const gchar *uri = NULL;

  g_assert (GIG_IS_URL_ENTRY (self));
  g_assert (WEBKIT_IS_WEB_VIEW (self->web_view));

  set_is_editing (self, FALSE);
  set_is_focused (self, FALSE);

  uri = webkit_web_view_get_uri (self->web_view);
  set_text (self, uri);
}

static void
entry_activate_cb (GigUrlEntry *self,
                   GtkEntry *entry)
{
  const gchar *text;
  g_autofree gchar *uri = NULL;

  g_assert (GIG_IS_URL_ENTRY (self));
  g_assert (GTK_IS_ENTRY (entry));
  g_assert (WEBKIT_IS_WEB_VIEW (self->web_view));

  text = gtk_editable_get_text (GTK_EDITABLE (entry));
  if (!text || text[0] == '\0')
    return;

  uri = gig_utils_fixup_uri (text);
  if (!uri)
    uri = gig_utils_build_search_uri (text);

  webkit_web_view_load_uri (self->web_view, uri);

  gtk_widget_grab_focus (GTK_WIDGET (self->web_view));
}

static void
web_view_uri_changed_cb (GigUrlEntry *self,
                         GParamSpec *pspec,
                         WebKitWebView *web_view)
{
  const gchar *uri;

  g_assert (GIG_IS_URL_ENTRY (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  uri = webkit_web_view_get_uri (web_view);
  set_text (self, uri);

  update_primary_icon (self);
}

static void
web_view_is_loading_changed_cb (GigUrlEntry *self,
                                GParamSpec *pspec,
                                WebKitWebView *web_view)
{
  g_assert (GIG_IS_URL_ENTRY (self));

  update_progress_fraction (self);
}

static void
web_view_estimated_load_progress_changed_cb (GigUrlEntry *self,
                                             GParamSpec *pspec,
                                             WebKitWebView *web_view)
{
  g_assert (GIG_IS_URL_ENTRY (self));

  update_progress_fraction (self);
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
      g_value_set_string (value, self->primary_icon_name);
      break;

    case PROP_PROGRESS_FRACTION:
      g_value_set_double (value, self->progress_fraction);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gig_url_entry_class_init (GigUrlEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = gig_url_entry_dispose;
  object_class->finalize = gig_url_entry_finalize;
  object_class->get_property = gig_url_entry_get_property;

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

  self->web_view_signals = g_signal_group_new (WEBKIT_TYPE_WEB_VIEW);

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
}

GigUrlEntry *
gig_url_entry_new (void)
{
  return g_object_new (GIG_TYPE_URL_ENTRY, NULL);
}

void
gig_url_entry_set_web_view (GigUrlEntry *self,
                            WebKitWebView *web_view)
{
  const gchar *uri = NULL;

  g_return_if_fail (GIG_IS_URL_ENTRY (self));
  g_return_if_fail (!web_view || WEBKIT_IS_WEB_VIEW (web_view));

  gtk_widget_set_sensitive (GTK_WIDGET (self->entry), web_view != NULL);

  if (web_view)
    uri = webkit_web_view_get_uri (web_view);

  set_text (self, uri);

  g_set_object (&self->web_view, web_view);

  g_signal_group_set_target (self->web_view_signals, web_view);

  update_primary_icon (self);
  update_progress_fraction (self);

  if (web_view && !uri)
    gtk_widget_grab_focus (GTK_WIDGET (self->entry));
}
