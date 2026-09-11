#pragma once

#include "gig-window.h"

G_BEGIN_DECLS

struct _GigWindow
{
  AdwApplicationWindow parent_instance;

  GtkButton *stop_reload_button;
  GigAddressBar *address_bar;
  AdwTabView *tab_view;

  GigPage *selected_page;

  GSignalGroup *web_view_signals;
};

void gig_window_class_init_actions (GigWindowClass *klass);

void gig_window_init_actions (GigWindow *self);

void gig_window_update_actions (GigWindow *self, GigWebView *web_view);

G_END_DECLS
