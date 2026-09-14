#pragma once

#include "gig-application.h"

G_BEGIN_DECLS

struct _GigApplication
{
  AdwApplication parent_instance;
};

void gig_application_init_actions (GigApplication *self);

G_END_DECLS
