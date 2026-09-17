#include "gig-web-view.h"

#include "gig-uri-utils.h"

struct _GigWebView
{
  WebKitWebView parent_instance;

  gchar *pending_address;

  GigConnectionSecurityLevel connection_security_level;
};

G_DEFINE_FINAL_TYPE (GigWebView, gig_web_view, WEBKIT_TYPE_WEB_VIEW)

enum
{
  PROP_0,
  PROP_ADDRESS,
  PROP_IS_BLANK,
  PROP_CAN_GO_BACK,
  PROP_CAN_GO_FORWARD,
  PROP_CONNECTION_SECURITY_LEVEL,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void gig_web_view_set_connection_security_level (GigWebView *self,
                                                        GigConnectionSecurityLevel connection_security_level);

static WebKitContextMenuItem *
find_item_in_context_menu (WebKitContextMenu *context_menu,
                           WebKitContextMenuAction action,
                           guint *index)
{
  GList *items, *iter;

  items = webkit_context_menu_get_items (context_menu);
  for (iter = items; iter; iter = g_list_next (iter))
    {
      WebKitContextMenuItem *item = (WebKitContextMenuItem *) iter->data;

      if (webkit_context_menu_item_get_stock_action (item) == action)
        {
          if (index)
            *index = g_list_index (items, item);
          return item;
        }
    }

  return NULL;
}

static void
remove_context_menu_item (WebKitContextMenu *context_menu,
                          WebKitContextMenuAction action)
{
  WebKitContextMenuItem *item = find_item_in_context_menu (context_menu, action, NULL);

  if (item)
    webkit_context_menu_remove (context_menu, item);
}

static void
rename_context_menu_item (WebKitContextMenu *context_menu,
                          WebKitContextMenuAction action,
                          const gchar *new_label)
{
  guint index;
  WebKitContextMenuItem *item = find_item_in_context_menu (context_menu, action, &index);

  if (item)
    {
      webkit_context_menu_remove (context_menu, item);

      item = webkit_context_menu_item_new_from_stock_action_with_label (action,
                                                                        new_label);

      webkit_context_menu_insert (context_menu, item, index);
    }
}

static gboolean
web_view_context_menu_cb (GigWebView *self,
                          WebKitContextMenu *context_menu,
                          WebKitHitTestResult *hit_test_result,
                          WebKitWebView *web_view)
{
  g_assert (GIG_IS_WEB_VIEW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  if (webkit_hit_test_result_context_is_link (hit_test_result))
    {
      remove_context_menu_item (context_menu,
                                WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK);

      rename_context_menu_item (context_menu,
                                WEBKIT_CONTEXT_MENU_ACTION_COPY_LINK_TO_CLIPBOARD,
                                "Copy Link Address");

      rename_context_menu_item (context_menu,
                                WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK_IN_NEW_WINDOW,
                                "Open Link in New Tab");
    }

  if (webkit_hit_test_result_context_is_image (hit_test_result))
    {
      rename_context_menu_item (context_menu,
                                WEBKIT_CONTEXT_MENU_ACTION_OPEN_IMAGE_IN_NEW_WINDOW,
                                "Open Image in New Tab");
    }

  return FALSE;
}

static void
web_view_load_changed_cb (GigWebView *self,
                          WebKitLoadEvent load_event,
                          WebKitWebView *web_view)
{
  switch (load_event)
    {
    case WEBKIT_LOAD_COMMITTED:
      {
        GTlsCertificate *certificate = NULL;
        GTlsCertificateFlags tls_errors = 0;

        if (webkit_web_view_get_tls_info (web_view, &certificate, &tls_errors) &&
            tls_errors == 0)
          gig_web_view_set_connection_security_level (self,
                                                      GIG_CONNECTION_SECURITY_LEVEL_SECURE);
        else
          gig_web_view_set_connection_security_level (self,
                                                      GIG_CONNECTION_SECURITY_LEVEL_INSECURE);
        break;
      }

    default:
      break;
    }
}

static void
web_view_uri_changed_cb (GigWebView *self,
                         GParamSpec *pspec,
                         WebKitWebView *web_view)
{
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ADDRESS]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_IS_BLANK]);
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
                           "context-menu",
                           G_CALLBACK (web_view_context_menu_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (web_view,
                           "load-changed",
                           G_CALLBACK (web_view_load_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);

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

    case PROP_IS_BLANK:
      g_value_set_boolean (value, gig_web_view_is_blank (self));
      break;

    case PROP_CAN_GO_BACK:
      g_value_set_boolean (value, webkit_web_view_can_go_back (web_view));
      break;

    case PROP_CAN_GO_FORWARD:
      g_value_set_boolean (value, webkit_web_view_can_go_forward (web_view));
      break;

    case PROP_CONNECTION_SECURITY_LEVEL:
      g_value_set_enum (value, gig_web_view_get_connection_security_level (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

GType
gig_connection_security_level_get_type (void)
{
  static GType type_id = 0;

  static const GEnumValue values[] = {
    { GIG_CONNECTION_SECURITY_LEVEL_TBD, "GIG_CONNECTION_SECURITY_LEVEL_TBD", "tbd" },
    { GIG_CONNECTION_SECURITY_LEVEL_INSECURE, "GIG_CONNECTION_SECURITY_LEVEL_INSECURE", "insecure" },
    { GIG_CONNECTION_SECURITY_LEVEL_SECURE, "GIG_CONNECTION_SECURITY_LEVEL_SECURE", "secure" },
    { 0 }
  };

  if (G_UNLIKELY (!type_id))
    type_id = g_enum_register_static ("GigConnectionSecurityLevel", values);

  return type_id;
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

  properties[PROP_IS_BLANK] =
      g_param_spec_boolean ("is-blank",
                            NULL, NULL,
                            TRUE,
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

  properties[PROP_CONNECTION_SECURITY_LEVEL] =
      g_param_spec_enum ("connection-security-level",
                         NULL, NULL,
                         GIG_TYPE_CONNECTION_SECURITY_LEVEL,
                         GIG_CONNECTION_SECURITY_LEVEL_TBD,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
gig_web_view_init (GigWebView *self)
{
  self->connection_security_level = GIG_CONNECTION_SECURITY_LEVEL_TBD;
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

  if ((uri = gig_fixup_uri (address)) == NULL)
    uri = gig_build_search_uri (address);

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

GigConnectionSecurityLevel
gig_web_view_get_connection_security_level (GigWebView *self)
{
  g_return_val_if_fail (GIG_IS_WEB_VIEW (self), GIG_CONNECTION_SECURITY_LEVEL_TBD);

  return self->connection_security_level;
}

static void
gig_web_view_set_connection_security_level (GigWebView *self,
                                            GigConnectionSecurityLevel connection_security_level)
{
  g_return_if_fail (GIG_IS_WEB_VIEW (self));

  if (self->connection_security_level == connection_security_level)
    return;

  self->connection_security_level = connection_security_level;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CONNECTION_SECURITY_LEVEL]);
}
