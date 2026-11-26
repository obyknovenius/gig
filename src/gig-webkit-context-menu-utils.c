#include "gig-webkit-context-menu-utils.h"

static WebKitContextMenuItem *
gig_webkit_context_menu_find_stock_action (WebKitContextMenu *menu,
                                           WebKitContextMenuAction action,
                                           guint *index)
{
  GList *items, *iter;
  guint i = 0;

  g_assert (WEBKIT_IS_CONTEXT_MENU (menu));

  items = webkit_context_menu_get_items (menu);
  for (iter = items; iter; iter = g_list_next (iter), i++)
    {
      WebKitContextMenuItem *item = (WebKitContextMenuItem *) iter->data;

      if (webkit_context_menu_item_get_stock_action (item) == action)
        {
          if (index)
            *index = i;
          return item;
        }
    }

  return NULL;
}

void
gig_webkit_context_menu_remove_stock_action (WebKitContextMenu *menu,
                                             WebKitContextMenuAction action)
{
  WebKitContextMenuItem *item;

  g_return_if_fail (WEBKIT_IS_CONTEXT_MENU (menu));

  item = gig_webkit_context_menu_find_stock_action (menu, action, NULL);

  if (item)
    webkit_context_menu_remove (menu, item);
}

void
gig_webkit_context_menu_rename_stock_action (WebKitContextMenu *menu,
                                             WebKitContextMenuAction action,
                                             const gchar *new_label)
{
  guint index;
  WebKitContextMenuItem *item;

  g_return_if_fail (WEBKIT_IS_CONTEXT_MENU (menu));

  item = gig_webkit_context_menu_find_stock_action (menu, action, &index);

  if (item)
    {
      webkit_context_menu_remove (menu, item);

      item = webkit_context_menu_item_new_from_stock_action_with_label (action,
                                                                        new_label);

      webkit_context_menu_insert (menu, item, index);
    }
}

void
gig_webkit_context_menu_prepend_gaction (WebKitContextMenu *menu,
                                         GAction *action,
                                         const gchar *label,
                                         GVariant *target)
{
  WebKitContextMenuItem *item;

  g_return_if_fail (WEBKIT_IS_CONTEXT_MENU (menu));

  item = webkit_context_menu_item_new_from_gaction (action, label, target);

  webkit_context_menu_prepend (menu, item);
}
