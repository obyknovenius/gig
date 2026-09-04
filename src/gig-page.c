#include "gig-page.h"

#include <webkit/webkit.h>

struct _GigPage
{
  GtkWidget parent_instance;

  WebKitWebView *web_view;
};

G_DEFINE_FINAL_TYPE (GigPage, gig_page, GTK_TYPE_WIDGET)

enum
{
  PROP_0,
  PROP_WEB_VIEW,
  PROP_TITLE,
  PROP_ICON,
  PROP_IS_LOADING,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
web_view_uri_changed_cb (GigPage *self,
                         GParamSpec *pspec,
                         WebKitWebView *web_view)
{
  g_assert (GIG_IS_PAGE (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);
}

static void
web_view_title_changed_cb (GigPage *self,
                           GParamSpec *pspec,
                           WebKitWebView *web_view)
{
  g_assert (GIG_IS_PAGE (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);
}

static void
web_view_favicon_changed_cb (GigPage *self,
                             GParamSpec *pspec,
                             WebKitWebView *web_view)
{
  g_assert (GIG_IS_PAGE (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ICON]);
}

static void
web_view_is_loading_changed_cb (GigPage *self,
                                GParamSpec *pspec,
                                WebKitWebView *web_view)
{
  g_assert (GIG_IS_PAGE (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_IS_LOADING]);
}

static void
gig_page_constructed (GObject *object)
{
  GigPage *self = GIG_PAGE (object);

  G_OBJECT_CLASS (gig_page_parent_class)->constructed (object);

  g_assert (self->web_view != NULL);

  gtk_widget_set_hexpand (GTK_WIDGET (self->web_view), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (self->web_view), TRUE);

  gtk_widget_set_parent (GTK_WIDGET (self->web_view), GTK_WIDGET (self));

  g_signal_connect_object (self->web_view,
                           "notify::uri",
                           G_CALLBACK (web_view_uri_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->web_view,
                           "notify::title",
                           G_CALLBACK (web_view_title_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->web_view,
                           "notify::favicon",
                           G_CALLBACK (web_view_favicon_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->web_view,
                           "notify::is-loading",
                           G_CALLBACK (web_view_is_loading_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);
}

static void
gig_page_dispose (GObject *object)
{
  GigPage *self = GIG_PAGE (object);

  if (self->web_view)
    {
      g_signal_handlers_disconnect_by_data (self->web_view, self);

      g_clear_pointer ((GtkWidget **) &self->web_view, gtk_widget_unparent);
    }

  G_OBJECT_CLASS (gig_page_parent_class)->dispose (object);
}

static void
gig_page_set_property (GObject *object,
                       guint prop_id,
                       const GValue *value,
                       GParamSpec *pspec)
{
  GigPage *self = GIG_PAGE (object);
  WebKitWebView *web_view = NULL;

  switch (prop_id)
    {
    case PROP_WEB_VIEW:
      web_view = g_value_get_object (value);
      g_assert (web_view != NULL);
      self->web_view = web_view;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gig_page_get_property (GObject *object,
                       guint prop_id,
                       GValue *value,
                       GParamSpec *pspec)
{
  GigPage *self = GIG_PAGE (object);

  switch (prop_id)
    {
    case PROP_WEB_VIEW:
      g_value_set_object (value, gig_page_get_web_view (self));
      break;
    case PROP_TITLE:
      g_value_set_string (value, gig_page_get_title (self));
      break;

    case PROP_ICON:
      g_value_set_object (value, gig_page_get_icon (self));
      break;

    case PROP_IS_LOADING:
      g_value_set_boolean (value, gig_page_get_is_loading (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gboolean
gig_page_grab_focus (GtkWidget *widget)
{
  GigPage *self = GIG_PAGE (widget);

  if (webkit_web_view_get_uri (self->web_view) == NULL)
    return FALSE;

  return gtk_widget_grab_focus (GTK_WIDGET (self->web_view));
}

static void
gig_page_class_init (GigPageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = gig_page_constructed;
  object_class->dispose = gig_page_dispose;
  object_class->set_property = gig_page_set_property;
  object_class->get_property = gig_page_get_property;
  widget_class->grab_focus = gig_page_grab_focus;

  properties[PROP_WEB_VIEW] =
      g_param_spec_object ("web-view",
                           NULL, NULL,
                           WEBKIT_TYPE_WEB_VIEW,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  properties[PROP_TITLE] =
      g_param_spec_string ("title", NULL, NULL,
                           NULL,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_ICON] =
      g_param_spec_object ("icon",
                           NULL, NULL,
                           GDK_TYPE_TEXTURE,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_IS_LOADING] =
      g_param_spec_boolean ("is-loading",
                            NULL, NULL,
                            FALSE,
                            G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
gig_page_init (GigPage *self)
{
}

GigPage *
gig_page_new (void)
{
  WebKitWebView *web_view = g_object_new (WEBKIT_TYPE_WEB_VIEW,
                                          NULL);

  return g_object_new (GIG_TYPE_PAGE,
                       "web-view", web_view,
                       NULL);
}

GigPage *
gig_page_new_with_related_web_view (WebKitWebView *related_web_view)
{
  g_return_val_if_fail (WEBKIT_IS_WEB_VIEW (related_web_view), NULL);

  WebKitWebView *web_view = g_object_new (WEBKIT_TYPE_WEB_VIEW,
                                          "related-view", related_web_view,
                                          NULL);

  return g_object_new (GIG_TYPE_PAGE,
                       "web-view", web_view,
                       NULL);
}

WebKitWebView *
gig_page_get_web_view (GigPage *self)
{
  g_return_val_if_fail (GIG_IS_PAGE (self), NULL);

  return self->web_view;
}

const gchar *
gig_page_get_title (GigPage *self)
{
  const gchar *title = NULL;

  g_return_val_if_fail (GIG_IS_PAGE (self), NULL);

  title = webkit_web_view_get_title (self->web_view);
  if (title && title[0] != '\0')
    return title;

  title = webkit_web_view_get_uri (self->web_view);
  if (title && title[0] != '\0')
    return title;

  return "New Tab";
}

GdkTexture *
gig_page_get_icon (GigPage *self)
{
  g_return_val_if_fail (GIG_IS_PAGE (self), NULL);

  return webkit_web_view_get_favicon (self->web_view);
}

gboolean
gig_page_get_is_loading (GigPage *self)
{
  g_return_val_if_fail (GIG_IS_PAGE (self), FALSE);

  return webkit_web_view_is_loading (self->web_view);
}
