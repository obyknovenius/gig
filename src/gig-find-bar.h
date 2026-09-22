#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_FIND_BAR (gig_find_bar_get_type ())

G_DECLARE_FINAL_TYPE (GigFindBar, gig_find_bar, GIG, FIND_BAR, GtkWidget)

GigFindBar *gig_find_bar_new (void);

WebKitFindController *gig_find_bar_get_find_controller (GigFindBar *self);

void gig_find_bar_set_find_controller (GigFindBar *self,
                                       WebKitFindController *find_controller);

void gig_find_bar_search (GigFindBar *self);

void gig_find_bar_search_finish (GigFindBar *self);

G_END_DECLS
