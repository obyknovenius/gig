#pragma once

#include <webkit/webkit.h>

G_BEGIN_DECLS

void gig_webkit_context_menu_remove_stock_action (WebKitContextMenu *menu,
                                                  WebKitContextMenuAction menu_action);

void gig_webkit_context_menu_rename_stock_action (WebKitContextMenu *menu,
                                                  WebKitContextMenuAction menu_action,
                                                  const gchar *new_label);

void gig_webkit_context_menu_replace_stock_action (WebKitContextMenu *menu,
                                                   WebKitContextMenuAction menu_action,
                                                   GAction *action,
                                                   const gchar *label,
                                                   GVariant *target);

void gig_webkit_context_menu_prepend_action (WebKitContextMenu *menu,
                                             GAction *action,
                                             const gchar *label,
                                             GVariant *target);

G_END_DECLS
