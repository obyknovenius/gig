#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_FIND_BAR (gig_find_bar_get_type ())

G_DECLARE_FINAL_TYPE (GigFindBar, gig_find_bar, GIG, FIND_BAR, GtkWidget)

GigFindBar *gig_find_bar_new (WebKitFindController *find_controller);

G_END_DECLS
