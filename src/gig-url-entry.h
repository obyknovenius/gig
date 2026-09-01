#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_URL_ENTRY (gig_url_entry_get_type ())

G_DECLARE_FINAL_TYPE (GigUrlEntry, gig_url_entry, GIG, URL_ENTRY, GtkWidget)

GigUrlEntry *gig_url_entry_new (void);

void gig_url_entry_set_page (GigUrlEntry *self, GigPage *page);

G_END_DECLS
