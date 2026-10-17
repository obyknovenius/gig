#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_SETTINGS (gig_settings_get_type ())

G_DECLARE_FINAL_TYPE (GigSettings, gig_settings, GIG, SETTINGS, GObject)

GigSettings *gig_settings_new (void);

void gig_settings_get_window_state (GigSettings *self,
                                    gint *width,
                                    gint *height,
                                    gboolean *maximized);

void gig_settings_set_window_state (GigSettings *self,
                                    gint width,
                                    gint height,
                                    gboolean maximized);

G_END_DECLS
