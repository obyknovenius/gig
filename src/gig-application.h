#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_APPLICATION (gig_application_get_type ())

G_DECLARE_FINAL_TYPE (GigApplication, gig_application, GIG, APPLICATION, AdwApplication)

GigApplication *gig_application_new ();

G_END_DECLS
