#pragma once

#include "gig-types.h"

G_BEGIN_DECLS

#define GIG_TYPE_ADDRESS_BAR (gig_address_bar_get_type ())

G_DECLARE_FINAL_TYPE (GigAddressBar, gig_address_bar, GIG, ADDRESS_BAR, GtkWidget)

GtkWidget *gig_address_bar_new (void);

void gig_address_bar_set_web_view (GigAddressBar *self,
                                   GigWebView *web_view);

G_END_DECLS
