#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_APPLICATION (gig_application_get_type ())
#define GIG_APPLICATION_DEFAULT (GIG_APPLICATION (g_application_get_default ()))

G_DECLARE_FINAL_TYPE (GigApplication, gig_application, GIG, APPLICATION, AdwApplication)

GigApplication *gig_application_new ();

GigSettings *gig_application_get_settings (GigApplication *self);

GigWindow *gig_application_get_current_window (GigApplication *self);

G_END_DECLS
