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

static void gig_find_bar_set_find_controller (GigFindBar *self,
                                              WebKitFindController *find_controller);

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
search_entry_search_changed_cb (GigFindBar *self,
                                GtkSearchEntry *search_entry)
{
  const gchar *text;
  WebKitFindOptions options = WEBKIT_FIND_OPTIONS_WRAP_AROUND | WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE;

  g_assert (GIG_IS_FIND_BAR (self));
  g_assert (WEBKIT_IS_FIND_CONTROLLER (self->find_controller));

  text = gtk_editable_get_text (GTK_EDITABLE (search_entry));

  webkit_find_controller_count_matches (self->find_controller,
                                        text, options, G_MAXUINT);

  webkit_find_controller_search (self->find_controller,
                                 text, options, G_MAXUINT);
}

static void
gig_find_bar_constructed (GObject *object)
{
  GigFindBar *self = GIG_FIND_BAR (object);

  G_OBJECT_CLASS (gig_find_bar_parent_class)->constructed (object);

  g_assert (WEBKIT_IS_FIND_CONTROLLER (self->find_controller));

  g_signal_connect_object (self->find_controller,
                           "counted-matches",
                           G_CALLBACK (find_controller_counted_matches_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_assert (GTK_IS_SEARCH_ENTRY (self->search_entry));

  g_signal_connect_object (self->search_entry,
                           "search-changed",
                           G_CALLBACK (search_entry_search_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);
}

static void
gig_find_bar_dispose (GObject *object)
{
  GigFindBar *self = GIG_FIND_BAR (object);

  gtk_widget_dispose_template (GTK_WIDGET (self), GIG_TYPE_FIND_BAR);

  G_OBJECT_CLASS (gig_find_bar_parent_class)->dispose (object);
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

static gboolean
gig_find_bar_grab_focus (GtkWidget *widget)
{
  GigFindBar *self = GIG_FIND_BAR (widget);

  g_assert (GIG_IS_FIND_BAR (self));

  return gtk_widget_grab_focus (GTK_WIDGET (self->search_entry));
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

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/obyknovenius/Gig/ui/gig-find-bar.ui");

  gtk_widget_class_bind_template_child (widget_class, GigFindBar, center_box);
  gtk_widget_class_bind_template_child (widget_class, GigFindBar, search_entry);
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

  if (self->find_controller == find_controller)
    return;

  self->find_controller = find_controller;
}
