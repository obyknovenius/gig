#include "gig-web-view.h"

#include "gig-application.h"

#include "history/gig-history-service.h"

struct _GigWebView
{
  WebKitWebView parent_instance;

  gchar *pending_uri;

  GigConnectionSecurityLevel connection_security_level;
};

G_DEFINE_FINAL_TYPE (GigWebView, gig_web_view, WEBKIT_TYPE_WEB_VIEW)

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

enum
{
  PROP_0,
  PROP_PENDING_URI,
  PROP_DISPLAY_URI,
  PROP_CONNECTION_SECURITY_LEVEL,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
gig_web_view_set_connection_security_level (GigWebView *self,
                                            GigConnectionSecurityLevel connection_security_level)
{
  g_assert (GIG_IS_WEB_VIEW (self));

  if (self->connection_security_level == connection_security_level)
    return;

  self->connection_security_level = connection_security_level;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CONNECTION_SECURITY_LEVEL]);
}

static void
gig_web_view_record_visit (GigWebView *self)
{
  GigHistoryService *history_service;
  const gchar *uri;
  g_autoptr (GError) error = NULL;

  g_assert (GIG_IS_WEB_VIEW (self));

  history_service = gig_application_get_history_service (GIG_APPLICATION_DEFAULT);
  uri = webkit_web_view_get_uri (WEBKIT_WEB_VIEW (self));

  if (!history_service)
    return;

  /* TODO: Don't record WebKit's error pages, which commit under the
   * failed URI. See how Epiphany freezes history in load_error_page.
   *
   * TODO: Reloads and back/forward navigations are recorded as regular
   * visits, so detaching a tab (which reloads the page) adds one too.
   *
   * TODO: Same-document navigations (pushState, fragment changes) don't
   * emit load-changed and are not recorded. They do notify::uri. */
  if (!gig_history_service_add_visit (history_service, uri, &error))
    g_warning ("Could not record visit to %s: %s", uri, error->message);
}

static void
gig_web_view_update_history_title (GigWebView *self)
{
  GigHistoryService *history_service;
  const gchar *uri;
  const gchar *title;
  g_autoptr (GError) error = NULL;

  g_assert (GIG_IS_WEB_VIEW (self));

  history_service = gig_application_get_history_service (GIG_APPLICATION_DEFAULT);
  uri = webkit_web_view_get_uri (WEBKIT_WEB_VIEW (self));
  title = webkit_web_view_get_title (WEBKIT_WEB_VIEW (self));

  if (!history_service)
    return;

  /* TODO: notify::title fires on every change, so pages that update their
   * title from JavaScript (unread counters and the like) cause a write each
   * time, and the last title wins. Firefox does the same, but skips unchanged
   * titles and error pages and writes asynchronously. Chromium only stores
   * titles set while loading or within 5 seconds after, at most 10 times per
   * navigation (HistoryTabHelper::TitleWasSet). */
  if (!gig_history_service_set_title (history_service, uri, title, &error))
    g_warning ("Could not update title of %s: %s", uri, error->message);
}

static void
gig_web_view_web_view_load_changed_cb (GigWebView *self,
                                       WebKitLoadEvent load_event,
                                       WebKitWebView *web_view)
{
  g_assert (GIG_IS_WEB_VIEW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  switch (load_event)
    {
    case WEBKIT_LOAD_COMMITTED:
      {
        GTlsCertificate *certificate = NULL;
        GTlsCertificateFlags tls_errors = 0;

        if (webkit_web_view_get_tls_info (web_view, &certificate, &tls_errors) && tls_errors == 0)
          gig_web_view_set_connection_security_level (self, GIG_CONNECTION_SECURITY_LEVEL_SECURE);
        else
          gig_web_view_set_connection_security_level (self, GIG_CONNECTION_SECURITY_LEVEL_INSECURE);

        gig_web_view_record_visit (self);
        break;
      }

    default:
      break;
    }
}

static void
gig_web_view_notify_web_view_uri_cb (GigWebView *self,
                                     GParamSpec *pspec,
                                     WebKitWebView *web_view)
{
  g_assert (GIG_IS_WEB_VIEW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  g_clear_pointer (&self->pending_uri, g_free);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DISPLAY_URI]);
}

static void
gig_web_view_notify_web_view_title_cb (GigWebView *self,
                                       GParamSpec *pspec,
                                       WebKitWebView *web_view)
{
  g_assert (GIG_IS_WEB_VIEW (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  gig_web_view_update_history_title (self);
}

static void
gig_web_view_constructed (GObject *object)
{
  GigWebView *self = (GigWebView *) object;
  WebKitWebView *web_view = WEBKIT_WEB_VIEW (self);

  g_assert (GIG_IS_WEB_VIEW (self));

  G_OBJECT_CLASS (gig_web_view_parent_class)->constructed (object);

  g_signal_connect_object (web_view,
                           "load-changed",
                           G_CALLBACK (gig_web_view_web_view_load_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (web_view,
                           "notify::uri",
                           G_CALLBACK (gig_web_view_notify_web_view_uri_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (web_view,
                           "notify::title",
                           G_CALLBACK (gig_web_view_notify_web_view_title_cb),
                           self,
                           G_CONNECT_SWAPPED);
}

static void
gig_web_view_finalize (GObject *object)
{
  GigWebView *self = (GigWebView *) object;

  g_assert (GIG_IS_WEB_VIEW (self));

  g_clear_pointer (&self->pending_uri, g_free);

  G_OBJECT_CLASS (gig_web_view_parent_class)->finalize (object);
}

static void
gig_web_view_set_property (GObject *object,
                           guint prop_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
  GigWebView *self = (GigWebView *) object;

  g_assert (GIG_IS_WEB_VIEW (self));

  switch (prop_id)
    {
    case PROP_PENDING_URI:
      g_set_str (&self->pending_uri, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gig_web_view_get_property (GObject *object,
                           guint prop_id,
                           GValue *value,
                           GParamSpec *pspec)
{
  GigWebView *self = (GigWebView *) object;

  g_assert (GIG_IS_WEB_VIEW (self));

  switch (prop_id)
    {
    case PROP_DISPLAY_URI:
      g_value_set_string (value, gig_web_view_get_display_uri (self));
      break;

    case PROP_CONNECTION_SECURITY_LEVEL:
      g_value_set_enum (value, gig_web_view_get_connection_security_level (self));
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
  object_class->set_property = gig_web_view_set_property;
  object_class->get_property = gig_web_view_get_property;
  object_class->finalize = gig_web_view_finalize;

  properties[PROP_PENDING_URI] =
      g_param_spec_string ("pending-uri",
                           NULL, NULL,
                           NULL,
                           G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  properties[PROP_DISPLAY_URI] =
      g_param_spec_string ("display-uri",
                           NULL, NULL,
                           NULL,
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

WebKitSettings *
gig_web_view_get_default_settings (void)
{
  static WebKitSettings *settings = NULL;

  if (!settings)
    {
      settings = webkit_settings_new_with_settings (
          "enable-back-forward-navigation-gestures", TRUE,
          NULL);

      g_object_add_weak_pointer (G_OBJECT (settings), (gpointer *) &settings);
    }

  return settings;
}

GigWebView *
gig_web_view_new (void)
{
  return g_object_new (GIG_TYPE_WEB_VIEW,
                       "settings", gig_web_view_get_default_settings (),
                       NULL);
}

GigWebView *
gig_web_view_new_for_navigation_action (WebKitNavigationAction *navigation_action,
                                        WebKitWebView *related_view)
{
  WebKitURIRequest *request;

  g_return_val_if_fail (navigation_action != NULL, NULL);
  g_return_val_if_fail (WEBKIT_IS_WEB_VIEW (related_view), NULL);

  request = webkit_navigation_action_get_request (navigation_action);

  return g_object_new (GIG_TYPE_WEB_VIEW,
                       "settings", gig_web_view_get_default_settings (),
                       "related-view", related_view,
                       "pending-uri", webkit_uri_request_get_uri (request),
                       NULL);
}

GigWebView *
gig_web_view_duplicate (GigWebView *self)
{
  GigWebView *new_view;
  WebKitWebViewSessionState *session_state;
  WebKitBackForwardList *back_forward_list;
  WebKitBackForwardListItem *item;

  g_return_val_if_fail (GIG_IS_WEB_VIEW (self), NULL);

  new_view = gig_web_view_new ();

  session_state = webkit_web_view_get_session_state (WEBKIT_WEB_VIEW (self));

  webkit_web_view_restore_session_state (WEBKIT_WEB_VIEW (new_view), session_state);
  webkit_web_view_session_state_unref (session_state);

  back_forward_list = webkit_web_view_get_back_forward_list (WEBKIT_WEB_VIEW (new_view));
  item = webkit_back_forward_list_get_current_item (back_forward_list);
  if (item)
    webkit_web_view_go_to_back_forward_list_item (WEBKIT_WEB_VIEW (new_view), item);
  else
    {
      const gchar *uri = gig_web_view_get_display_uri (self);

      if (uri)
        webkit_web_view_load_uri (WEBKIT_WEB_VIEW (new_view), uri);
    }

  return GIG_WEB_VIEW (new_view);
}

const gchar *
gig_web_view_get_display_uri (GigWebView *self)
{
  const gchar *uri;

  g_return_val_if_fail (GIG_IS_WEB_VIEW (self), NULL);

  uri = webkit_web_view_get_uri (WEBKIT_WEB_VIEW (self));
  if (uri)
    return uri;

  return self->pending_uri;
}

GigConnectionSecurityLevel
gig_web_view_get_connection_security_level (GigWebView *self)
{
  g_return_val_if_fail (GIG_IS_WEB_VIEW (self), GIG_CONNECTION_SECURITY_LEVEL_TBD);

  return self->connection_security_level;
}
