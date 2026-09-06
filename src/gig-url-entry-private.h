#pragma once

#include "gig-url-entry.h"

G_BEGIN_DECLS

void gig_url_entry_set_text (GigUrlEntry *self,
                             const gchar *text);

void gig_url_entry_set_editing (GigUrlEntry *self,
                                gboolean editing);

G_END_DECLS
