#include "gig-floating-bar.h"

struct _GigFloatingBar
{
  GtkWidget parent_instance;

  GtkRevealer *revealer;
  GtkLabel *label;

  guint show_timeout_id;
  guint hide_timeout_id;
};

G_DEFINE_FINAL_TYPE (GigFloatingBar, gig_floating_bar, GTK_TYPE_WIDGET)

static void
gig_floating_bar_show_timeout_cb (gpointer user_data)
{
  GigFloatingBar *self = user_data;

  g_assert (GIG_IS_FLOATING_BAR (self));

  self->show_timeout_id = 0;
  gtk_revealer_set_reveal_child (self->revealer, TRUE);
}

static void
gig_floating_bar_hide_timeout_cb (gpointer user_data)
{
  GigFloatingBar *self = user_data;

  g_assert (GIG_IS_FLOATING_BAR (self));

  self->hide_timeout_id = 0;
  gtk_revealer_set_reveal_child (self->revealer, FALSE);
}

static void
gig_floating_bar_dispose (GObject *object)
{
  GigFloatingBar *self = (GigFloatingBar *) object;

  g_assert (GIG_IS_FLOATING_BAR (self));

  g_clear_handle_id (&self->show_timeout_id, g_source_remove);
  g_clear_handle_id (&self->hide_timeout_id, g_source_remove);

  gtk_widget_dispose_template (GTK_WIDGET (self), GIG_TYPE_FLOATING_BAR);

  G_OBJECT_CLASS (gig_floating_bar_parent_class)->dispose (object);
}

static void
gig_floating_bar_class_init (GigFloatingBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = gig_floating_bar_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/obyknovenius/Gig/gig-floating-bar.ui");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "floatingbar");

  gtk_widget_class_bind_template_child (widget_class, GigFloatingBar, revealer);
  gtk_widget_class_bind_template_child (widget_class, GigFloatingBar, label);
}

static void
gig_floating_bar_init (GigFloatingBar *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

void
gig_floating_bar_set_text (GigFloatingBar *self,
                           const gchar *text)
{
  gboolean is_revealed;

  g_return_if_fail (GIG_IS_FLOATING_BAR (self));

  is_revealed = gtk_revealer_get_reveal_child (self->revealer);

  if (text && text[0] != '\0')
    {
      gtk_label_set_text (self->label, text);

      g_clear_handle_id (&self->hide_timeout_id, g_source_remove);

      if (!is_revealed && self->show_timeout_id == 0)
        self->show_timeout_id = g_timeout_add_once (80, gig_floating_bar_show_timeout_cb, self);
    }
  else
    {
      g_clear_handle_id (&self->show_timeout_id, g_source_remove);

      if (is_revealed && self->hide_timeout_id == 0)
        self->hide_timeout_id = g_timeout_add_once (250, gig_floating_bar_hide_timeout_cb, self);
    }
}
