#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_FIND_ENTRY (gig_find_entry_get_type ())

G_DECLARE_FINAL_TYPE (GigFindEntry, gig_find_entry, GIG, FIND_ENTRY, GtkWidget)

guint gig_find_entry_get_occurrence_count (GigFindEntry *self);
void gig_find_entry_set_occurrence_count (GigFindEntry *self,
                                          guint occurrence_count);

guint gig_find_entry_get_occurrence_position (GigFindEntry *self);
void gig_find_entry_set_occurrence_position (GigFindEntry *self,
                                             guint occurrence_position);

G_END_DECLS
