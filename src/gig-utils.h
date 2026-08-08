#pragma once

#include <glib.h>

G_BEGIN_DECLS

gchar *gig_utils_fixup_uri (const gchar *uri);

gchar *gig_utils_build_search_uri (const gchar *query);

G_END_DECLS
