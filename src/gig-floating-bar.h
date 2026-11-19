#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_FLOATING_BAR (gig_floating_bar_get_type ())

G_DECLARE_FINAL_TYPE (GigFloatingBar, gig_floating_bar, GIG, FLOATING_BAR, GtkWidget)

void gig_floating_bar_set_text (GigFloatingBar *self, const gchar *text);

G_END_DECLS
