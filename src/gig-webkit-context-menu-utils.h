#pragma once

#include <webkit/webkit.h>

G_BEGIN_DECLS

void gig_webkit_context_menu_remove_stock_action (WebKitContextMenu *menu,
                                                  WebKitContextMenuAction action);

void gig_webkit_context_menu_rename_stock_action (WebKitContextMenu *menu,
                                                  WebKitContextMenuAction action,
                                                  const gchar *new_label);

void gig_webkit_context_menu_prepend_gaction (WebKitContextMenu *menu,
                                              GAction *action,
                                              const gchar *label,
                                              GVariant *target);

G_END_DECLS
