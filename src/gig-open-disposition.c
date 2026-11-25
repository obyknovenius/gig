#include "config.h"

#include "gig-open-disposition.h"

GType
gig_open_disposition_get_type (void)
{
  static GType type_id = 0;

  static const GEnumValue values[] = {
    { GIG_OPEN_DISPOSITION_NEW_FOREGROUND_TAB, "GIG_OPEN_DISPOSITION_NEW_FOREGROUND_TAB", "foreground-tab" },
    { GIG_OPEN_DISPOSITION_NEW_BACKGROUND_TAB, "GIG_OPEN_DISPOSITION_NEW_BACKGROUND_TAB", "background-tab" },
    { GIG_OPEN_DISPOSITION_NEW_WINDOW, "GIG_OPEN_DISPOSITION_NEW_WINDOW", "new-window" },
    { 0 }
  };

  if (G_UNLIKELY (!type_id))
    type_id = g_enum_register_static ("GigOpenDisposition", values);

  return type_id;
}
