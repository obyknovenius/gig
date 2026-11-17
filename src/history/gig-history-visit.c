#include "gig-history-visit.h"

struct _GigHistoryVisit
{
  GObject parent_instance;

  gchar *uri;
  gchar *title;
  gint64 time;
};

G_DEFINE_FINAL_TYPE (GigHistoryVisit, gig_history_visit, G_TYPE_OBJECT)

static void
gig_history_visit_finalize (GObject *object)
{
  GigHistoryVisit *self = (GigHistoryVisit *) object;

  g_assert (GIG_IS_HISTORY_VISIT (self));

  g_clear_pointer (&self->uri, g_free);
  g_clear_pointer (&self->title, g_free);

  G_OBJECT_CLASS (gig_history_visit_parent_class)->finalize (object);
}

static void
gig_history_visit_class_init (GigHistoryVisitClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gig_history_visit_finalize;
}

static void
gig_history_visit_init (GigHistoryVisit *self)
{
}

GigHistoryVisit *
gig_history_visit_new (const gchar *uri,
                       const gchar *title,
                       gint64 time)
{
  GigHistoryVisit *self;

  g_return_val_if_fail (uri != NULL, NULL);

  self = g_object_new (GIG_TYPE_HISTORY_VISIT, NULL);
  self->uri = g_strdup (uri);
  self->title = g_strdup (title);
  self->time = time;

  return self;
}

const gchar *
gig_history_visit_get_uri (GigHistoryVisit *self)
{
  g_return_val_if_fail (GIG_IS_HISTORY_VISIT (self), NULL);

  return self->uri;
}

const gchar *
gig_history_visit_get_title (GigHistoryVisit *self)
{
  g_return_val_if_fail (GIG_IS_HISTORY_VISIT (self), NULL);

  return self->title;
}

gint64
gig_history_visit_get_time (GigHistoryVisit *self)
{
  g_return_val_if_fail (GIG_IS_HISTORY_VISIT (self), 0);

  return self->time;
}
