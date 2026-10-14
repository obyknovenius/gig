#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_WINDOW (gig_window_get_type ())

G_DECLARE_FINAL_TYPE (GigWindow, gig_window, GIG, WINDOW, AdwApplicationWindow)

// TODO: Return GtkWidget instead of GigWindow
GigWindow *gig_window_new (GigApplication *application);

GigWindow *gig_window_new_empty (GigApplication *application);

void gig_window_add_tab (GigWindow *self,
                         GigTab *tab,
                         GigTab *parent_tab,
                         gboolean set_selected);

void gig_window_insert_tab (GigWindow *self,
                            GigTab *tab,
                            gint position,
                            gboolean set_selected);

GigTab *gig_window_get_selected_tab (GigWindow *self);

G_END_DECLS
