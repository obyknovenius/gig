#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_WINDOW (gig_window_get_type ())

G_DECLARE_FINAL_TYPE (GigWindow, gig_window, GIG, WINDOW, AdwApplicationWindow)

GigWindow *gig_window_new (GtkApplication *app);

void gig_window_add_tab_page (GigWindow *self,
                              GigPage *page,
                              gboolean set_selected,
                              AdwTabPage *parent);

G_END_DECLS
