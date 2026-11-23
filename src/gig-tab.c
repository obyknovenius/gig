#include "config.h"

#include "gig-tab.h"

#include "gig-find-bar.h"
#include "gig-floating-bar.h"
#include "gig-web-view.h"

struct _GigTab
{
  GtkWidget parent_instance;

  AdwToolbarView *toolbar_view;
  GtkStack *stack;
  AdwStatusPage *new_tab_page;
  GigWebView *web_view;
  GigFloatingBar *floating_bar;
  GigFindBar *find_bar;

  gboolean is_empty;
};

G_DEFINE_FINAL_TYPE (GigTab, gig_tab, GTK_TYPE_WIDGET)

enum
{
  PROP_0,
  PROP_WEB_VIEW,
  PROP_TITLE,
  PROP_ICON,
  PROP_IS_EMPTY,
  PROP_IS_LOADING,
  PROP_CAN_RELOAD,
  PROP_CAN_GO_BACK,
  PROP_CAN_GO_FORWARD,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
gig_tab_set_web_view (GigTab *self,
                      GigWebView *web_view)
{
  WebKitFindController *find_controller;

  g_assert (GIG_IS_TAB (self));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  if (self->web_view == web_view)
    return;

  self->web_view = web_view;

  find_controller = webkit_web_view_get_find_controller (WEBKIT_WEB_VIEW (web_view));
  self->find_bar = gig_find_bar_new (find_controller);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_WEB_VIEW]);
}

static void
gig_tab_set_is_empty (GigTab *self,
                      gboolean is_empty)
{
  g_assert (GIG_IS_TAB (self));

  if (self->is_empty == is_empty)
    return;

  self->is_empty = is_empty;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_IS_EMPTY]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CAN_RELOAD]);
}

static void
gig_tab_notify_web_view_uri_cb (GigTab *self,
                                GParamSpec *pspec,
                                GigWebView *web_view)
{
  g_assert (GIG_IS_TAB (self));

  gtk_stack_set_visible_child (self->stack, GTK_WIDGET (self->web_view));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);
}

static void
gig_tab_notify_web_view_title_cb (GigTab *self,
                                  GParamSpec *pspec,
                                  GigWebView *web_view)
{
  g_assert (GIG_IS_TAB (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);
}

static void
gig_tab_notify_web_view_favicon_cb (GigTab *self,
                                    GParamSpec *pspec,
                                    GigWebView *web_view)
{
  g_assert (GIG_IS_TAB (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ICON]);
}

static void
gig_tab_notify_web_view_is_loading_cb (GigTab *self,
                                       GParamSpec *pspec,
                                       GigWebView *web_view)
{
  g_assert (GIG_IS_TAB (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_IS_LOADING]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CAN_RELOAD]);
}

static gboolean
gig_tab_web_view_enter_fullscreen_cb (GigTab *self,
                                      GigWebView *web_view)
{
  g_assert (GIG_IS_TAB (self));

  gtk_widget_set_visible (GTK_WIDGET (self->find_bar), FALSE);

  return FALSE;
}

static gboolean
gig_tab_web_view_leave_fullscreen_cb (GigTab *self,
                                      GigWebView *web_view)
{
  g_assert (GIG_IS_TAB (self));

  gtk_widget_set_visible (GTK_WIDGET (self->find_bar), TRUE);

  return FALSE;
}

static void
gig_tab_web_view_load_changed_cb (GigTab *self,
                                  WebKitLoadEvent load_event,
                                  WebKitWebView *web_view)
{
  g_assert (GIG_IS_TAB (self));

  if (load_event == WEBKIT_LOAD_COMMITTED)
    gig_tab_set_is_empty (self, FALSE);
}

static void
gig_tab_web_view_mouse_target_changed_cb (GigTab *self,
                                          WebKitHitTestResult *hit_test_result,
                                          guint modifiers,
                                          GigWebView *web_view)
{
  g_assert (GIG_IS_TAB (self));

  gig_floating_bar_set_text (self->floating_bar,
                             webkit_hit_test_result_get_link_uri (hit_test_result));
}

static void
gig_tab_back_forward_list_changed_cb (GigTab *self,
                                      WebKitBackForwardListItem *item_added,
                                      gpointer items_removed,
                                      WebKitBackForwardList *back_forward_list)
{
  g_assert (GIG_IS_TAB (self));
  g_assert (WEBKIT_IS_BACK_FORWARD_LIST (back_forward_list));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CAN_GO_BACK]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CAN_GO_FORWARD]);
}

static void
gig_tab_constructed (GObject *object)
{
  GigTab *self = (GigTab *) object;
  WebKitBackForwardList *back_forward_list;

  g_assert (GIG_IS_TAB (self));

  G_OBJECT_CLASS (gig_tab_parent_class)->constructed (object);

  g_assert (GIG_IS_WEB_VIEW (self->web_view));
  g_assert (GIG_IS_FIND_BAR (self->find_bar));

  adw_status_page_set_icon_name (self->new_tab_page,
                                 APP_ID "-symbolic");

  adw_toolbar_view_add_bottom_bar (self->toolbar_view,
                                   GTK_WIDGET (self->find_bar));

  gtk_stack_add_child (self->stack, GTK_WIDGET (self->web_view));

  if (gig_web_view_get_display_uri (self->web_view))
    gtk_stack_set_visible_child (self->stack, GTK_WIDGET (self->web_view));

  g_signal_connect_object (self->web_view,
                           "notify::uri",
                           G_CALLBACK (gig_tab_notify_web_view_uri_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->web_view,
                           "notify::title",
                           G_CALLBACK (gig_tab_notify_web_view_title_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->web_view,
                           "notify::favicon",
                           G_CALLBACK (gig_tab_notify_web_view_favicon_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->web_view,
                           "notify::is-loading",
                           G_CALLBACK (gig_tab_notify_web_view_is_loading_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->web_view,
                           "enter-fullscreen",
                           G_CALLBACK (gig_tab_web_view_enter_fullscreen_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->web_view,
                           "leave-fullscreen",
                           G_CALLBACK (gig_tab_web_view_leave_fullscreen_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->web_view,
                           "load-changed",
                           G_CALLBACK (gig_tab_web_view_load_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->web_view,
                           "mouse-target-changed",
                           G_CALLBACK (gig_tab_web_view_mouse_target_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  back_forward_list = webkit_web_view_get_back_forward_list (WEBKIT_WEB_VIEW (self->web_view));

  g_signal_connect_object (back_forward_list,
                           "changed",
                           G_CALLBACK (gig_tab_back_forward_list_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);
}

static void
gig_tab_dispose (GObject *object)
{
  GigTab *self = (GigTab *) object;

  g_assert (GIG_IS_TAB (self));

  if (self->web_view)
    {
      g_signal_handlers_disconnect_by_data (self->web_view, self);
      adw_toolbar_view_remove (self->toolbar_view, GTK_WIDGET (self->find_bar));
    }

  gtk_widget_dispose_template (GTK_WIDGET (self), GIG_TYPE_TAB);

  G_OBJECT_CLASS (gig_tab_parent_class)->dispose (object);
}

static void
gig_tab_set_property (GObject *object,
                      guint prop_id,
                      const GValue *value,
                      GParamSpec *pspec)
{
  GigTab *self = (GigTab *) object;

  g_assert (GIG_IS_TAB (self));

  switch (prop_id)
    {
    case PROP_WEB_VIEW:
      gig_tab_set_web_view (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gig_tab_get_property (GObject *object,
                      guint prop_id,
                      GValue *value,
                      GParamSpec *pspec)
{
  GigTab *self = (GigTab *) object;

  g_assert (GIG_IS_TAB (self));

  switch (prop_id)
    {
    case PROP_WEB_VIEW:
      g_value_set_object (value, gig_tab_get_web_view (self));
      break;

    case PROP_TITLE:
      g_value_set_string (value, gig_tab_get_title (self));
      break;

    case PROP_ICON:
      g_value_set_object (value, gig_tab_get_icon (self));
      break;

    case PROP_IS_EMPTY:
      g_value_set_boolean (value, gig_tab_is_empty (self));
      break;

    case PROP_IS_LOADING:
      g_value_set_boolean (value, gig_tab_is_loading (self));
      break;

    case PROP_CAN_RELOAD:
      g_value_set_boolean (value, gig_tab_can_reload (self));
      break;

    case PROP_CAN_GO_BACK:
      g_value_set_boolean (value, gig_tab_can_go_back (self));
      break;

    case PROP_CAN_GO_FORWARD:
      g_value_set_boolean (value, gig_tab_can_go_forward (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gboolean
gig_tab_grab_focus (GtkWidget *widget)
{
  GigTab *self = (GigTab *) widget;

  g_assert (GIG_IS_TAB (self));

  if (!gig_web_view_get_display_uri (self->web_view))
    return FALSE;

  return gtk_widget_grab_focus (GTK_WIDGET (self->web_view));
}

static void
gig_tab_class_init (GigTabClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = gig_tab_constructed;
  object_class->dispose = gig_tab_dispose;
  object_class->set_property = gig_tab_set_property;
  object_class->get_property = gig_tab_get_property;
  widget_class->grab_focus = gig_tab_grab_focus;

  properties[PROP_WEB_VIEW] =
      g_param_spec_object ("web-view",
                           NULL, NULL,
                           GIG_TYPE_WEB_VIEW,
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

  properties[PROP_IS_EMPTY] =
      g_param_spec_boolean ("is-empty",
                            NULL, NULL,
                            TRUE,
                            G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_IS_LOADING] =
      g_param_spec_boolean ("is-loading",
                            NULL, NULL,
                            FALSE,
                            G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_CAN_RELOAD] =
      g_param_spec_boolean ("can-reload",
                            NULL, NULL,
                            FALSE,
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

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/obyknovenius/Gig/gig-tab.ui");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

  gtk_widget_class_bind_template_child (widget_class, GigTab, toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, GigTab, stack);
  gtk_widget_class_bind_template_child (widget_class, GigTab, new_tab_page);
  gtk_widget_class_bind_template_child (widget_class, GigTab, floating_bar);

  g_type_ensure (GIG_TYPE_FLOATING_BAR);
}

static void
gig_tab_init (GigTab *self)
{
  self->is_empty = TRUE;

  gtk_widget_init_template (GTK_WIDGET (self));
}

GigTab *
gig_tab_new (void)
{
  GigWebView *web_view = gig_web_view_new ();

  return g_object_new (GIG_TYPE_TAB,
                       "web-view", web_view,
                       NULL);
}

GigTab *
gig_tab_new_with_web_view (GigWebView *web_view)
{
  return g_object_new (GIG_TYPE_TAB,
                       "web-view", web_view,
                       NULL);
}

GigWebView *
gig_tab_get_web_view (GigTab *self)
{
  g_return_val_if_fail (GIG_IS_TAB (self), NULL);

  return self->web_view;
}

const gchar *
gig_tab_get_title (GigTab *self)
{
  const gchar *title = NULL;

  g_return_val_if_fail (GIG_IS_TAB (self), NULL);

  title = webkit_web_view_get_title (WEBKIT_WEB_VIEW (self->web_view));
  if (title && title[0] != '\0')
    return title;

  title = gig_web_view_get_display_uri (self->web_view);
  if (title && title[0] != '\0')
    return title;

  return "New Tab";
}

GdkTexture *
gig_tab_get_icon (GigTab *self)
{
  g_return_val_if_fail (GIG_IS_TAB (self), NULL);

  return webkit_web_view_get_favicon (WEBKIT_WEB_VIEW (self->web_view));
}

gboolean
gig_tab_is_empty (GigTab *self)
{
  g_return_val_if_fail (GIG_IS_TAB (self), FALSE);

  return self->is_empty;
}

gboolean
gig_tab_is_loading (GigTab *self)
{
  g_return_val_if_fail (GIG_IS_TAB (self), FALSE);

  return webkit_web_view_is_loading (WEBKIT_WEB_VIEW (self->web_view));
}

gboolean
gig_tab_can_reload (GigTab *self)
{
  gboolean is_loading;

  g_return_val_if_fail (GIG_IS_TAB (self), FALSE);

  is_loading = webkit_web_view_is_loading (WEBKIT_WEB_VIEW (self->web_view));

  return !self->is_empty || is_loading;
}

gboolean
gig_tab_can_go_back (GigTab *self)
{
  g_return_val_if_fail (GIG_IS_TAB (self), FALSE);

  return webkit_web_view_can_go_back (WEBKIT_WEB_VIEW (self->web_view));
}

gboolean
gig_tab_can_go_forward (GigTab *self)
{
  g_return_val_if_fail (GIG_IS_TAB (self), FALSE);

  return webkit_web_view_can_go_forward (WEBKIT_WEB_VIEW (self->web_view));
}

void
gig_tab_reveal_find_bar (GigTab *self)
{
  g_return_if_fail (GIG_IS_TAB (self));
  g_return_if_fail (GIG_IS_FIND_BAR (self->find_bar));

  adw_toolbar_view_set_reveal_bottom_bars (self->toolbar_view, TRUE);

  gig_find_bar_search (self->find_bar);

  gtk_widget_grab_focus (GTK_WIDGET (self->find_bar));
}

void
gig_tab_dismiss_find_bar (GigTab *self)
{
  g_return_if_fail (GIG_IS_TAB (self));
  g_return_if_fail (GIG_IS_FIND_BAR (self->find_bar));

  adw_toolbar_view_set_reveal_bottom_bars (self->toolbar_view, FALSE);

  gig_find_bar_search_finish (self->find_bar);
}
