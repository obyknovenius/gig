#pragma once

#include "gig-window.h"

G_BEGIN_DECLS

struct _GigWindow
{
  AdwApplicationWindow parent_instance;

  AdwToolbarView *toolbar_view;
  AdwHeaderBar *header_bar;
  AdwTabBar *tab_bar;
  GtkButton *stop_reload_button;
  GigAddressBar *address_bar;
  AdwTabView *tab_view;

  AdwTabPage *menu_page;

  GSignalGroup *web_view_signals;
};

void gig_window_class_actions_init (GigWindowClass *klass);

void gig_window_actions_init (GigWindow *self);

void gig_window_actions_update (GigWindow *self);

G_END_DECLS
