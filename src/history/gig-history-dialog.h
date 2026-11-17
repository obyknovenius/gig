#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_HISTORY_DIALOG (gig_history_dialog_get_type ())

G_DECLARE_FINAL_TYPE (GigHistoryDialog, gig_history_dialog, GIG, HISTORY_DIALOG, AdwDialog)

GigHistoryDialog *gig_history_dialog_new (GigHistoryService *history_service);

G_END_DECLS
