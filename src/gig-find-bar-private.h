#pragma once

#include "gig-find-bar.h"

G_BEGIN_DECLS

struct _GigFindBar
{
  GtkWidget parent_instance;

  GtkCenterBox *center_box;
  GtkSearchEntry *entry;
  WebKitFindController *find_controller;
};

void gig_find_bar_class_actions_init (GigFindBarClass *klass);

void gig_find_bar_actions_init (GigFindBar *self);

G_END_DECLS
