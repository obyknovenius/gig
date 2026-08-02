#include "gig-window.h"

struct _GigWindow
{
  AdwApplicationWindow parent_instance;

  WebKitWebView *web_view;
};

G_DEFINE_TYPE (GigWindow, gig_window, ADW_TYPE_APPLICATION_WINDOW)

static void
gig_window_class_init (GigWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/obyknovenius/Gig/ui/gig-window.ui");

  gtk_widget_class_bind_template_child (widget_class, GigWindow, web_view);

  g_type_ensure (WEBKIT_TYPE_WEB_VIEW);
}

static void
gig_window_init (GigWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GigWindow *
gig_window_new (GtkApplication *app)
{
  return g_object_new (GIG_TYPE_WINDOW,
                       "application", app,
                       NULL);
}

WebKitWebView *
gig_window_get_web_view (GigWindow *self)
{
  g_return_val_if_fail (GIG_IS_WINDOW (self), NULL);

  return self->web_view;
}
