#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_WEB_VIEW (gig_web_view_get_type ())

G_DECLARE_FINAL_TYPE (GigWebView, gig_web_view, GIG, WEB_VIEW, WebKitWebView)

GtkWidget *gig_web_view_new (void);

GtkWidget *gig_web_view_new_with_related_view (WebKitWebView *related_view);

void gig_web_view_load_address (GigWebView *web_view,
                                const gchar *address);

void gig_web_view_set_pending_address (GigWebView *web_view,
                                       const gchar *pending_address);

const gchar *gig_web_view_get_address (GigWebView *web_view);

gboolean gig_web_view_is_blank (GigWebView *web_view);

G_END_DECLS
