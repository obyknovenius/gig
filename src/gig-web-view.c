#include "gig-web-view.h"

#include "gig-utils.h"

struct _GigWebView
{
  WebKitWebView parent_instance;

  gchar *pending_address;
};

G_DEFINE_FINAL_TYPE (GigWebView, gig_web_view, WEBKIT_TYPE_WEB_VIEW)

enum
{
  PROP_0,
  PROP_ADDRESS,
  PROP_CAN_GO_BACK,
  PROP_CAN_GO_FORWARD,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
web_view_uri_changed_cb (GigWebView *self,
                         GParamSpec *pspec,
                         WebKitWebView *web_view)
{
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ADDRESS]);
}

static void
back_forward_list_changed_cb (GigWebView *self,
                              WebKitBackForwardListItem *item_added,
                              gpointer items_removed,
                              WebKitBackForwardList *back_forward_list)
{
  GList *back_list = NULL;
  GList *forward_list = NULL;

  g_assert (GIG_IS_WEB_VIEW (self));
  g_assert (WEBKIT_IS_BACK_FORWARD_LIST (back_forward_list));

  if (webkit_back_forward_list_get_back_list (back_forward_list))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CAN_GO_BACK]);

  if (webkit_back_forward_list_get_forward_list (back_forward_list))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CAN_GO_FORWARD]);

  forward_list = webkit_back_forward_list_get_forward_list (back_forward_list);

  g_list_free (back_list);
  g_list_free (forward_list);
}

static void
gig_web_view_constructed (GObject *object)
{
  GigWebView *self = GIG_WEB_VIEW (object);
  WebKitWebView *web_view = WEBKIT_WEB_VIEW (self);
  WebKitBackForwardList *back_forward_list = NULL;

  G_OBJECT_CLASS (gig_web_view_parent_class)->constructed (object);

  g_signal_connect_object (web_view,
                           "notify::uri",
                           G_CALLBACK (web_view_uri_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  back_forward_list = webkit_web_view_get_back_forward_list (web_view);

  g_signal_connect_object (back_forward_list,
                           "changed",
                           G_CALLBACK (back_forward_list_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);
}

static void
gig_web_view_finalize (GObject *object)
{
  GigWebView *self = GIG_WEB_VIEW (object);

  g_clear_pointer (&self->pending_address, g_free);

  G_OBJECT_CLASS (gig_web_view_parent_class)->finalize (object);
}

static void
gig_web_view_get_property (GObject *object,
                           guint prop_id,
                           GValue *value,
                           GParamSpec *pspec)
{
  GigWebView *self = GIG_WEB_VIEW (object);
  WebKitWebView *web_view = WEBKIT_WEB_VIEW (self);

  switch (prop_id)
    {
    case PROP_ADDRESS:
      g_value_set_string (value, gig_web_view_get_address (self));
      break;

    case PROP_CAN_GO_BACK:
      g_value_set_boolean (value, webkit_web_view_can_go_back (web_view));
      break;

    case PROP_CAN_GO_FORWARD:
      g_value_set_boolean (value, webkit_web_view_can_go_forward (web_view));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gig_web_view_class_init (GigWebViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = gig_web_view_constructed;
  object_class->get_property = gig_web_view_get_property;
  object_class->finalize = gig_web_view_finalize;

  properties[PROP_ADDRESS] =
      g_param_spec_string ("address",
                           NULL, NULL,
                           NULL,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_CAN_GO_BACK] =
      g_param_spec_boolean ("can-go-back",
                            NULL, NULL,
                            FALSE,
                            G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_CAN_GO_FORWARD] =
      g_param_spec_boolean ("can-go-forward",
                            NULL, NULL,
                            FALSE,
                            G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
gig_web_view_init (GigWebView *self)
{
}

GtkWidget *
gig_web_view_new (void)
{
  return g_object_new (GIG_TYPE_WEB_VIEW, NULL);
}

GtkWidget *
gig_web_view_new_with_related_view (WebKitWebView *related_view)
{
  return g_object_new (GIG_TYPE_WEB_VIEW,
                       "related-view", related_view,
                       NULL);
}

void
gig_web_view_load_address (GigWebView *web_view,
                           const gchar *address)
{
  g_autofree gchar *uri = NULL;

  g_return_if_fail (GIG_IS_WEB_VIEW (web_view));
  g_return_if_fail (address != NULL);

  uri = gig_utils_fixup_uri (address);
  if (!uri)
    uri = gig_utils_build_search_uri (address);

  webkit_web_view_load_uri (WEBKIT_WEB_VIEW (web_view), uri);
}

void
gig_web_view_set_pending_address (GigWebView *web_view,
                                  const gchar *pending_address)
{
  g_return_if_fail (GIG_IS_WEB_VIEW (web_view));

  if (g_set_str (&web_view->pending_address, pending_address))
    g_object_notify_by_pspec (G_OBJECT (web_view), properties[PROP_ADDRESS]);
}

const gchar *
gig_web_view_get_address (GigWebView *self)
{
  const gchar *uri;

  g_return_val_if_fail (GIG_IS_WEB_VIEW (self), NULL);

  if ((uri = webkit_web_view_get_uri (WEBKIT_WEB_VIEW (self))))
    return uri;

  return self->pending_address;
}

gboolean
gig_web_view_is_blank (GigWebView *self)
{
  g_return_val_if_fail (GIG_IS_WEB_VIEW (self), FALSE);

  return gig_web_view_get_address (self) == NULL;
}
