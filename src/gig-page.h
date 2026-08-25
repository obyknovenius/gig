#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_PAGE (gig_page_get_type ())

G_DECLARE_FINAL_TYPE (GigPage, gig_page, GIG, PAGE, GtkWidget)

GigPage *gig_page_new (void);

void gig_page_load_uri (GigPage *self, const gchar *uri);
void gig_page_reload (GigPage *self);
void gig_page_stop_loading (GigPage *self);
void gig_page_go_back (GigPage *self);
void gig_page_go_forward (GigPage *self);

const gchar *gig_page_get_uri (GigPage *self);
const gchar *gig_page_get_title (GigPage *self);
GdkTexture *gig_page_get_icon (GigPage *self);
gboolean gig_page_get_is_loading (GigPage *self);
gboolean gig_page_can_go_back (GigPage *self);
gboolean gig_page_can_go_forward (GigPage *self);

G_END_DECLS
