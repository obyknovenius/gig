#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_TAB (gig_tab_get_type ())
#define GIG_TYPE_TAB_DISPOSITION (gig_tab_disposition_get_type ())

G_DECLARE_FINAL_TYPE (GigTab, gig_tab, GIG, TAB, GtkWidget)

typedef enum _GigTabDisposition
{
  GIG_TAB_DISPOSITION_BACKGROUND,
  GIG_TAB_DISPOSITION_FOREGROUND,
  GIG_TAB_DISPOSITION_NEW_WINDOW,
} GigTabDisposition;

GType gig_tab_disposition_get_type (void) G_GNUC_CONST;

GigTab *gig_tab_new (void);

GigTab *gig_tab_new_with_web_view (GigWebView *web_view);

GigWebView *gig_tab_get_web_view (GigTab *self);

const gchar *gig_tab_get_title (GigTab *self);

GdkTexture *gig_tab_get_icon (GigTab *self);

gboolean gig_tab_is_empty (GigTab *self);

gboolean gig_tab_is_loading (GigTab *self);

gboolean gig_tab_can_reload (GigTab *self);

gboolean gig_tab_can_go_back (GigTab *self);

gboolean gig_tab_can_go_forward (GigTab *self);

void gig_tab_reload (GigTab *self);

void gig_tab_stop_loading (GigTab *self);

void gig_tab_go_back (GigTab *self);

void gig_tab_go_forward (GigTab *self);

void gig_tab_reveal_find_bar (GigTab *self);

void gig_tab_dismiss_find_bar (GigTab *self);

G_END_DECLS
