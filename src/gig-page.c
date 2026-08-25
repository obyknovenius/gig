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
  PROP_URI,
  PROP_TITLE,
  PROP_ICON,
  PROP_IS_LOADING,
  PROP_CAN_GO_BACK,
  PROP_CAN_GO_FORWARD,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
web_view_uri_changed_cb (GigPage *self,
                         GParamSpec *pspec,
                         WebKitWebView *web_view)
{
  g_assert (GIG_IS_PAGE (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_URI]);
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
back_forward_list_changed_cb (GigPage *self,
                              WebKitBackForwardListItem *item_added,
                              gpointer items_removed,
                              WebKitBackForwardList *back_forward_list)
{
  g_assert (GIG_IS_PAGE (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CAN_GO_BACK]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CAN_GO_FORWARD]);
}

static void
gig_page_dispose (GObject *object)
{
  GigPage *self = GIG_PAGE (object);
  WebKitBackForwardList *back_forward_list;

  back_forward_list = webkit_web_view_get_back_forward_list (self->web_view);
  g_signal_handlers_disconnect_by_data (back_forward_list, self);

  g_signal_handlers_disconnect_by_data (self->web_view, self);

  gtk_widget_dispose_template (GTK_WIDGET (self), GIG_TYPE_PAGE);

  G_OBJECT_CLASS (gig_page_parent_class)->dispose (object);
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
    case PROP_URI:
      g_value_set_string (value, gig_page_get_uri (self));
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

    case PROP_CAN_GO_BACK:
      g_value_set_boolean (value, gig_page_can_go_back (self));
      break;

    case PROP_CAN_GO_FORWARD:
      g_value_set_boolean (value, gig_page_can_go_forward (self));
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

  object_class->dispose = gig_page_dispose;
  object_class->get_property = gig_page_get_property;
  widget_class->grab_focus = gig_page_grab_focus;

  properties[PROP_URI] = g_param_spec_string ("uri",
                                              NULL, NULL,
                                              NULL,
                                              (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  properties[PROP_TITLE] = g_param_spec_string ("title",
                                                NULL, NULL,
                                                NULL,
                                                (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  properties[PROP_ICON] = g_param_spec_object ("icon",
                                               NULL, NULL,
                                               GDK_TYPE_TEXTURE,
                                               (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  properties[PROP_IS_LOADING] = g_param_spec_boolean ("is-loading",
                                                      NULL, NULL,
                                                      FALSE,
                                                      (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  properties[PROP_CAN_GO_BACK] = g_param_spec_boolean ("can-go-back",
                                                       NULL, NULL,
                                                       FALSE,
                                                       (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  properties[PROP_CAN_GO_FORWARD] = g_param_spec_boolean ("can-go-forward",
                                                          NULL, NULL,
                                                          FALSE,
                                                          (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/obyknovenius/Gig/ui/gig-page.ui");

  gtk_widget_class_bind_template_child (widget_class, GigPage, web_view);

  g_type_ensure (WEBKIT_TYPE_WEB_VIEW);
}

static void
gig_page_init (GigPage *self)
{
  WebKitBackForwardList *back_forward_list;

  gtk_widget_init_template (GTK_WIDGET (self));

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

  back_forward_list = webkit_web_view_get_back_forward_list (self->web_view);

  g_signal_connect_object (back_forward_list,
                           "changed",
                           G_CALLBACK (back_forward_list_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);
}

GigPage *
gig_page_new (void)
{
  return g_object_new (GIG_TYPE_PAGE, NULL);
}

void
gig_page_load_uri (GigPage *self,
                   const gchar *uri)
{
  g_return_if_fail (GIG_IS_PAGE (self));
  g_return_if_fail (uri != NULL);

  webkit_web_view_load_uri (self->web_view, uri);
}

void
gig_page_reload (GigPage *self)
{
  g_return_if_fail (GIG_IS_PAGE (self));

  webkit_web_view_reload (self->web_view);
}

void
gig_page_stop_loading (GigPage *self)
{
  g_return_if_fail (GIG_IS_PAGE (self));

  webkit_web_view_stop_loading (self->web_view);
}

void
gig_page_go_back (GigPage *self)
{
  g_return_if_fail (GIG_IS_PAGE (self));

  webkit_web_view_go_back (self->web_view);
}

void
gig_page_go_forward (GigPage *self)
{
  g_return_if_fail (GIG_IS_PAGE (self));

  webkit_web_view_go_forward (self->web_view);
}

WebKitWebView *
gig_page_get_web_view (GigPage *self)
{
  g_return_val_if_fail (GIG_IS_PAGE (self), NULL);

  return self->web_view;
}

const gchar *
gig_page_get_uri (GigPage *self)
{
  g_return_val_if_fail (GIG_IS_PAGE (self), NULL);

  return webkit_web_view_get_uri (self->web_view);
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

gboolean
gig_page_can_go_back (GigPage *self)
{
  g_return_val_if_fail (GIG_IS_PAGE (self), FALSE);

  return webkit_web_view_can_go_back (self->web_view);
}

gboolean
gig_page_can_go_forward (GigPage *self)
{
  g_return_val_if_fail (GIG_IS_PAGE (self), FALSE);

  return webkit_web_view_can_go_forward (self->web_view);
}
