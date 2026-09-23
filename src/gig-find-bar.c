#include "gig-find-bar-private.h"

#include "gig-web-view.h"

G_DEFINE_FINAL_TYPE (GigFindBar, gig_find_bar, GTK_TYPE_WIDGET)

enum
{
  PROP_0,
  PROP_FIND_CONTROLLER,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
find_controller_counted_matches_cb (GigFindBar *self,
                                    guint count,
                                    WebKitFindController *find_controller)
{
  g_assert (GIG_IS_FIND_BAR (self));

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "find.previous", count > 1);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "find.next", count > 1);
}

static void
entry_search_changed_cb (GigFindBar *self,
                         GtkSearchEntry *entry)
{
  g_assert (GIG_IS_FIND_BAR (self));

  gig_find_bar_search (self);
}

static void
entry_stop_search_cb (GigFindBar *self,
                      GtkSearchEntry *entry)
{
  g_assert (GIG_IS_FIND_BAR (self));

  gtk_widget_activate_action (GTK_WIDGET (self), "win.dismiss-find", NULL);
}

static void
gig_find_bar_constructed (GObject *object)
{
  GigFindBar *self = GIG_FIND_BAR (object);

  G_OBJECT_CLASS (gig_find_bar_parent_class)->constructed (object);

  g_assert (GTK_IS_SEARCH_ENTRY (self->entry));

  g_signal_connect_object (self->entry,
                           "search-changed",
                           G_CALLBACK (entry_search_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->entry,
                           "stop-search",
                           G_CALLBACK (entry_stop_search_cb),
                           self,
                           G_CONNECT_SWAPPED);
}

static void
gig_find_bar_dispose (GObject *object)
{
  GigFindBar *self = GIG_FIND_BAR (object);

  g_signal_group_set_target (self->find_controller_signals, NULL);

  gtk_widget_dispose_template (GTK_WIDGET (self), GIG_TYPE_FIND_BAR);

  G_OBJECT_CLASS (gig_find_bar_parent_class)->dispose (object);
}

static void
gig_find_bar_finalize (GObject *object)
{
  GigFindBar *self = GIG_FIND_BAR (object);

  g_assert (GIG_IS_FIND_BAR (self));

  g_clear_object (&self->find_controller_signals);
  g_clear_object (&self->find_controller);

  G_OBJECT_CLASS (gig_find_bar_parent_class)->finalize (object);
}

static void
gig_find_bar_set_property (GObject *object,
                           guint prop_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
  GigFindBar *self = GIG_FIND_BAR (object);

  switch (prop_id)
    {
    case PROP_FIND_CONTROLLER:
      gig_find_bar_set_find_controller (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gig_find_bar_get_property (GObject *object,
                           guint prop_id,
                           GValue *value,
                           GParamSpec *pspec)
{
  GigFindBar *self = GIG_FIND_BAR (object);

  switch (prop_id)
    {
    case PROP_FIND_CONTROLLER:
      g_value_set_object (value, gig_find_bar_get_find_controller (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gboolean
gig_find_bar_grab_focus (GtkWidget *widget)
{
  GigFindBar *self = GIG_FIND_BAR (widget);

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
  object_class->finalize = gig_find_bar_finalize;
  object_class->set_property = gig_find_bar_set_property;
  object_class->get_property = gig_find_bar_get_property;
  widget_class->grab_focus = gig_find_bar_grab_focus;

  properties[PROP_FIND_CONTROLLER] =
      g_param_spec_object ("find-controller",
                           NULL, NULL,
                           WEBKIT_TYPE_FIND_CONTROLLER,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gig_find_bar_class_actions_init (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/obyknovenius/Gig/gig-find-bar.ui");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "findbar");

  gtk_widget_class_bind_template_child (widget_class, GigFindBar, center_box);
  gtk_widget_class_bind_template_child (widget_class, GigFindBar, entry);
}

static void
gig_find_bar_init (GigFindBar *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gig_find_bar_actions_init (self);

  self->find_controller_signals = g_signal_group_new (WEBKIT_TYPE_FIND_CONTROLLER);

  g_signal_group_connect_object (self->find_controller_signals,
                                 "counted-matches",
                                 G_CALLBACK (find_controller_counted_matches_cb),
                                 self,
                                 G_CONNECT_SWAPPED);
}

GigFindBar *
gig_find_bar_new (void)
{
  return g_object_new (GIG_TYPE_FIND_BAR, NULL);
}

WebKitFindController *
gig_find_bar_get_find_controller (GigFindBar *self)
{
  g_assert (GIG_IS_FIND_BAR (self));

  return self->find_controller;
}

void
gig_find_bar_set_find_controller (GigFindBar *self,
                                  WebKitFindController *find_controller)
{
  g_assert (GIG_IS_FIND_BAR (self));

  if (!g_set_object (&self->find_controller, find_controller))
    return;

  g_signal_group_set_target (self->find_controller_signals,
                             self->find_controller);

  gtk_widget_set_sensitive (GTK_WIDGET (self->entry),
                            self->find_controller != NULL);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FIND_CONTROLLER]);
}

void
gig_find_bar_search (GigFindBar *self)
{
  const gchar *text;
  WebKitFindOptions options = WEBKIT_FIND_OPTIONS_WRAP_AROUND | WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE;

  g_assert (GIG_IS_FIND_BAR (self));
  g_assert (WEBKIT_IS_FIND_CONTROLLER (self->find_controller));

  text = gtk_editable_get_text (GTK_EDITABLE (self->entry));

  webkit_find_controller_count_matches (self->find_controller,
                                        text, options, G_MAXUINT);

  webkit_find_controller_search (self->find_controller,
                                 text, options, G_MAXUINT);
}

void
gig_find_bar_search_finish (GigFindBar *self)
{
  g_assert (GIG_IS_FIND_BAR (self));
  g_assert (WEBKIT_IS_FIND_CONTROLLER (self->find_controller));

  webkit_find_controller_search_finish (self->find_controller);
}
