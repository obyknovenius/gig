#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define GIG_TYPE_OPEN_DISPOSITION (gig_open_disposition_get_type ())

typedef enum _GigOpenDisposition
{
  GIG_OPEN_DISPOSITION_NEW_FOREGROUND_TAB,
  GIG_OPEN_DISPOSITION_NEW_BACKGROUND_TAB,
  GIG_OPEN_DISPOSITION_NEW_WINDOW,
} GigOpenDisposition;

GType gig_open_disposition_get_type (void) G_GNUC_CONST;

G_END_DECLS
