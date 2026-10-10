#include "gig-find-bar-private.h"

#include "gig-find-entry.h"
#include "gig-web-view.h"

G_DEFINE_FINAL_TYPE (GigFindBar, gig_find_bar, GTK_TYPE_WIDGET)

enum
{
  PROP_0,
  PROP_FIND_CONTROLLER,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void gig_find_bar_set_find_controller (GigFindBar *self,
                                              WebKitFindController *find_controller);

static void
reset (GigFindBar *self)
{
  g_assert (GIG_IS_FIND_BAR (self));

  gig_find_entry_set_occurrence_count (self->entry, 0);
  gig_find_entry_set_occurrence_position (self->entry, 0);

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "find.previous", FALSE);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "find.next", FALSE);
}

static void
find_controller_found_text_cb (GigFindBar *self,
                               guint match_count,
                               WebKitFindController *find_controller)
{
  g_assert (GIG_IS_FIND_BAR (self));

  gig_find_entry_set_occurrence_count (self->entry, match_count);

  if (gig_find_entry_get_occurrence_position (self->entry) == 0 && match_count > 0)
    gig_find_entry_set_occurrence_position (self->entry, 1);

  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "find.previous",
                                 match_count > 1);

  gtk_widget_action_set_enabled (GTK_WIDGET (self),
                                 "find.next",
                                 match_count > 1);
}

static void
find_controller_failed_to_find_text_cb (GigFindBar *self,
                                        WebKitFindController *find_controller)
{
  g_assert (GIG_IS_FIND_BAR (self));

  reset (self);
}

static void
entry_focus_enter_cb (GigFindBar *self,
                      GtkEventControllerFocus *controller)
{
  g_assert (GIG_IS_FIND_BAR (self));

  gtk_editable_select_region (GTK_EDITABLE (self->entry), 0, -1);
}

static void
entry_focus_leave_cb (GigFindBar *self,
                      GtkEventControllerFocus *controller)
{
  g_assert (GIG_IS_FIND_BAR (self));

  gtk_editable_set_position (GTK_EDITABLE (self->entry), -1);
}

static void
entry_search_changed_cb (GigFindBar *self,
                         GigFindEntry *entry)
{
  g_assert (GIG_IS_FIND_BAR (self));

  gig_find_bar_search (self);
}

static void
entry_stop_search_cb (GigFindBar *self,
                      GigFindEntry *entry)
{
  g_assert (GIG_IS_FIND_BAR (self));

  gtk_widget_activate_action (GTK_WIDGET (self), "tab.find-finish", NULL);
}

static void
gig_find_bar_constructed (GObject *object)
{
  GigFindBar *self = (GigFindBar *) object;

  g_assert (GIG_IS_FIND_BAR (self));

  G_OBJECT_CLASS (gig_find_bar_parent_class)->constructed (object);

  g_assert (WEBKIT_IS_FIND_CONTROLLER (self->find_controller));

  g_signal_connect_object (self->find_controller,
                           "found-text",
                           G_CALLBACK (find_controller_found_text_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->find_controller,
                           "failed-to-find-text",
                           G_CALLBACK (find_controller_failed_to_find_text_cb),
                           self,
                           G_CONNECT_SWAPPED);
}

static void
gig_find_bar_dispose (GObject *object)
{
  GigFindBar *self = (GigFindBar *) object;

  g_assert (GIG_IS_FIND_BAR (self));

  if (self->find_controller)
    g_signal_handlers_disconnect_by_data (self->find_controller, self);
  g_clear_object (&self->find_controller);

  gtk_widget_dispose_template (GTK_WIDGET (self), GIG_TYPE_FIND_BAR);

  G_OBJECT_CLASS (gig_find_bar_parent_class)->dispose (object);
}

static void
gig_find_bar_set_property (GObject *object,
                           guint prop_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
  GigFindBar *self = (GigFindBar *) object;

  g_assert (GIG_IS_FIND_BAR (self));

  switch (prop_id)
    {
    case PROP_FIND_CONTROLLER:
      gig_find_bar_set_find_controller (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gboolean
gig_find_bar_grab_focus (GtkWidget *widget)
{
  GigFindBar *self = (GigFindBar *) widget;

  g_assert (GIG_IS_FIND_BAR (self));

  return gtk_widget_grab_focus (GTK_WIDGET (self->entry));
}

static void
gig_find_bar_class_init (GigFindBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = gig_find_bar_constructed;
  object_class->dispose = gig_find_bar_dispose;
  object_class->set_property = gig_find_bar_set_property;
  widget_class->grab_focus = gig_find_bar_grab_focus;

  properties[PROP_FIND_CONTROLLER] =
      g_param_spec_object ("find-controller",
                           NULL, NULL,
                           WEBKIT_TYPE_FIND_CONTROLLER,
                           G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gig_find_bar_class_actions_init (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/obyknovenius/Gig/gig-find-bar.ui");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "findbar");

  gtk_widget_class_bind_template_child (widget_class, GigFindBar, center_box);
  gtk_widget_class_bind_template_child (widget_class, GigFindBar, entry);

  gtk_widget_class_bind_template_callback (widget_class, entry_search_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, entry_stop_search_cb);
  gtk_widget_class_bind_template_callback (widget_class, entry_focus_enter_cb);
  gtk_widget_class_bind_template_callback (widget_class, entry_focus_leave_cb);

  g_type_ensure (GIG_TYPE_FIND_ENTRY);
}

static void
gig_find_bar_init (GigFindBar *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gig_find_bar_actions_init (self);
}

GigFindBar *
gig_find_bar_new (WebKitFindController *find_controller)
{
  return g_object_new (GIG_TYPE_FIND_BAR,
                       "find-controller", find_controller,
                       NULL);
}

static void
gig_find_bar_set_find_controller (GigFindBar *self,
                                  WebKitFindController *find_controller)
{
  g_assert (GIG_IS_FIND_BAR (self));
  g_assert (WEBKIT_IS_FIND_CONTROLLER (find_controller));

  g_set_object (&self->find_controller, find_controller);
}

void
gig_find_bar_search (GigFindBar *self)
{
  const gchar *search_text;
  WebKitFindOptions options = WEBKIT_FIND_OPTIONS_WRAP_AROUND | WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE;

  g_return_if_fail (GIG_IS_FIND_BAR (self));
  g_return_if_fail (WEBKIT_IS_FIND_CONTROLLER (self->find_controller));

  reset (self);

  search_text = gtk_editable_get_text (GTK_EDITABLE (self->entry));

  webkit_find_controller_search (self->find_controller,
                                 search_text, options, G_MAXUINT);
}

void
gig_find_bar_search_finish (GigFindBar *self)
{
  g_return_if_fail (GIG_IS_FIND_BAR (self));
  g_return_if_fail (WEBKIT_IS_FIND_CONTROLLER (self->find_controller));

  webkit_find_controller_search_finish (self->find_controller);

  reset (self);
}
