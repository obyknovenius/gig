#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_HISTORY_VISIT (gig_history_visit_get_type ())

G_DECLARE_FINAL_TYPE (GigHistoryVisit, gig_history_visit, GIG, HISTORY_VISIT, GObject)

GigHistoryVisit *gig_history_visit_new (const gchar *uri,
                                        const gchar *title,
                                        gint64 time);

const gchar *gig_history_visit_get_uri (GigHistoryVisit *self);

const gchar *gig_history_visit_get_title (GigHistoryVisit *self);

gint64 gig_history_visit_get_time (GigHistoryVisit *self);

G_END_DECLS
