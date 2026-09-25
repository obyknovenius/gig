#include "gig-find-entry.h"

struct _GigFindEntry
{
  GtkWidget parent_instance;

  guint search_delay;
  guint search_timeout_id;

  GtkText *text;
  GtkLabel *occurrence_label;
  GtkImage *clear_image;

  guint occurrence_count;
  guint occurrence_position;
};

static void editable_iface_init (GtkEditableInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (GigFindEntry, gig_find_entry, GTK_TYPE_WIDGET, G_IMPLEMENT_INTERFACE (GTK_TYPE_EDITABLE, editable_iface_init))

enum
{
  PROP_0,
  PROP_OCCURRENCE_COUNT,
  PROP_OCCURRENCE_POSITION,
  N_PROPS,
};

enum
{
  SEARCH_CHANGED,
  STOP_SEARCH,
  N_SIGNALS
};

static GParamSpec *properties[N_PROPS];
static guint signals[N_SIGNALS];

static void search_timeout_cb (gpointer user_data);

static void
reset_timeout (GigFindEntry *self)
{
  g_clear_handle_id (&self->search_timeout_id, g_source_remove);

  self->search_timeout_id = g_timeout_add_once (self->search_delay,
                                                search_timeout_cb,
                                                self);
}

static void
update_occurrence_label (GigFindEntry *self)
{
  g_assert (GIG_IS_FIND_ENTRY (self));

  if (self->occurrence_count == 0)
    {
      gtk_label_set_label (self->occurrence_label, NULL);
    }
  else
    {
      g_autofree char *str = g_strdup_printf ("%u of %u",
                                              self->occurrence_position,
                                              self->occurrence_count);

      gtk_label_set_label (self->occurrence_label, str);
    }
}

static void
search_timeout_cb (gpointer user_data)
{
  GigFindEntry *self = user_data;

  g_assert (GIG_IS_FIND_ENTRY (self));

  self->search_timeout_id = 0;

  g_signal_emit (self, signals[SEARCH_CHANGED], 0);
}

static void
text_changed_cb (GigFindEntry *self,
                 GtkText *text)
{
#if GTK_CHECK_VERSION(4, 24, 0)
  g_autofree char *str = NULL;
#else
  const char *str;
#endif

  g_assert (GIG_IS_FIND_ENTRY (self));
  g_assert (GTK_IS_TEXT (text));

#if GTK_CHECK_VERSION(4, 24, 0)
  str = gtk_editable_get_complete_text (GTK_EDITABLE (self));
#else
  str = gtk_editable_get_text (GTK_EDITABLE (self));
#endif

  if (str == NULL || str[0] == '\0')
    {
      gtk_widget_set_visible (GTK_WIDGET (self->clear_image), FALSE);

      g_clear_handle_id (&self->search_timeout_id, g_source_remove);
      g_signal_emit (self, signals[SEARCH_CHANGED], 0);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->clear_image), TRUE);

      reset_timeout (self);
    }
}

static void
clear_pressed_cb (GigFindEntry *self,
                  int n_press,
                  double x,
                  double y,
                  GtkGestureClick *click)
{
  g_assert (GIG_IS_FIND_ENTRY (self));

  gtk_editable_set_text (GTK_EDITABLE (self), "");
}

static void
gig_find_entry_dispose (GObject *object)
{
  GigFindEntry *self = GIG_FIND_ENTRY (object);
  GtkWidget *child;

  g_clear_handle_id (&self->search_timeout_id, g_source_remove);

  gtk_editable_finish_delegate (GTK_EDITABLE (self));

  gtk_widget_dispose_template (GTK_WIDGET (self), GIG_TYPE_FIND_ENTRY);

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self))))
    gtk_widget_unparent (child);

  G_OBJECT_CLASS (gig_find_entry_parent_class)->dispose (object);
}

static void
gig_find_entry_get_property (GObject *object,
                             guint prop_id,
                             GValue *value,
                             GParamSpec *pspec)
{
  GigFindEntry *self = GIG_FIND_ENTRY (object);

  if (gtk_editable_delegate_get_property (object, prop_id, value, pspec))
    return;

  switch (prop_id)
    {
    case PROP_OCCURRENCE_COUNT:
      g_value_set_uint (value, gig_find_entry_get_occurrence_count (self));
      break;

    case PROP_OCCURRENCE_POSITION:
      g_value_set_uint (value, gig_find_entry_get_occurrence_position (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gig_find_entry_set_property (GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
  GigFindEntry *self = GIG_FIND_ENTRY (object);

  if (gtk_editable_delegate_set_property (object, prop_id, value, pspec))
    return;

  switch (prop_id)
    {
    case PROP_OCCURRENCE_COUNT:
      gig_find_entry_set_occurrence_count (self, g_value_get_uint (value));
      break;

    case PROP_OCCURRENCE_POSITION:
      gig_find_entry_set_occurrence_position (self, g_value_get_uint (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gboolean
gig_find_entry_grab_focus (GtkWidget *widget)
{
  GigFindEntry *self = GIG_FIND_ENTRY (widget);

  g_assert (GIG_IS_FIND_ENTRY (self));

  return gtk_widget_grab_focus (GTK_WIDGET (self->text));
}

static void
gig_find_entry_class_init (GigFindEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = gig_find_entry_get_property;
  object_class->set_property = gig_find_entry_set_property;
  object_class->dispose = gig_find_entry_dispose;
  widget_class->grab_focus = gig_find_entry_grab_focus;

  properties[PROP_OCCURRENCE_COUNT] =
      g_param_spec_uint ("occurrence-count",
                         NULL, NULL,
                         0, G_MAXUINT,
                         0,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_OCCURRENCE_POSITION] =
      g_param_spec_uint ("occurrence-position",
                         NULL, NULL,
                         0, G_MAXUINT,
                         0,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_editable_install_properties (object_class, N_PROPS);

  signals[SEARCH_CHANGED] =
      g_signal_new ("search-changed",
                    G_OBJECT_CLASS_TYPE (object_class),
                    G_SIGNAL_RUN_LAST,
                    0,
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE, 0);

  signals[STOP_SEARCH] =
      g_signal_new ("stop-search",
                    G_OBJECT_CLASS_TYPE (object_class),
                    G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                    0,
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE, 0);

  gtk_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Escape, GDK_NO_MODIFIER_MASK,
                                       "stop-search",
                                       NULL);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/obyknovenius/Gig/gig-find-entry.ui");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BOX_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "entry");
  gtk_widget_class_set_accessible_role (widget_class, GTK_ACCESSIBLE_ROLE_SEARCH_BOX);

  gtk_widget_class_bind_template_child (widget_class, GigFindEntry, text);
  gtk_widget_class_bind_template_child (widget_class, GigFindEntry, occurrence_label);
  gtk_widget_class_bind_template_child (widget_class, GigFindEntry, clear_image);

  gtk_widget_class_bind_template_callback (widget_class, text_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, clear_pressed_cb);
}

static void
gig_find_entry_init (GigFindEntry *self)
{
  self->search_delay = 150;

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_editable_init_delegate (GTK_EDITABLE (self));
}

static GtkEditable *
gig_find_entry_get_delegate (GtkEditable *editable)
{
  return GTK_EDITABLE (GIG_FIND_ENTRY (editable)->text);
}

static void
editable_iface_init (GtkEditableInterface *iface)
{
  iface->get_delegate = gig_find_entry_get_delegate;
}

guint
gig_find_entry_get_occurrence_count (GigFindEntry *self)
{
  g_return_val_if_fail (GIG_IS_FIND_ENTRY (self), 0);

  return self->occurrence_count;
}

void
gig_find_entry_set_occurrence_count (GigFindEntry *self,
                                     guint occurrence_count)
{
  g_return_if_fail (GIG_IS_FIND_ENTRY (self));

  if (self->occurrence_count == occurrence_count)
    return;

  self->occurrence_count = occurrence_count;

  update_occurrence_label (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_OCCURRENCE_COUNT]);
}

guint
gig_find_entry_get_occurrence_position (GigFindEntry *self)
{
  g_return_val_if_fail (GIG_IS_FIND_ENTRY (self), 0);

  return self->occurrence_position;
}

void
gig_find_entry_set_occurrence_position (GigFindEntry *self,
                                        guint occurrence_position)
{
  g_return_if_fail (GIG_IS_FIND_ENTRY (self));

  if (self->occurrence_position == occurrence_position)
    return;

  self->occurrence_position = occurrence_position;

  update_occurrence_label (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_OCCURRENCE_POSITION]);
}
