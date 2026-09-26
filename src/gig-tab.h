#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_TAB (gig_tab_get_type ())

G_DECLARE_FINAL_TYPE (GigTab, gig_tab, GIG, TAB, GtkWidget)

GtkWidget *gig_tab_new (void);

GtkWidget *gig_tab_new_with_web_view (GigWebView *web_view);

GigWebView *gig_tab_get_web_view (GigTab *self);

const gchar *gig_tab_get_title (GigTab *self);

GdkTexture *gig_tab_get_icon (GigTab *self);

gboolean gig_tab_get_is_loading (GigTab *self);

void gig_tab_reveal_find_bar (GigTab *self);

void gig_tab_dismiss_find_bar (GigTab *self);

G_END_DECLS
