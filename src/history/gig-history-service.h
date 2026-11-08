#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_HISTORY_SERVICE (gig_history_service_get_type ())

G_DECLARE_FINAL_TYPE (GigHistoryService, gig_history_service, GIG, HISTORY_SERVICE, GObject)

GigHistoryService *gig_history_service_new (const gchar *path,
                                            GError **error);

G_END_DECLS
