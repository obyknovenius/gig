#include "gig-history-service.h"

#include <sqlite3.h>

struct _GigHistoryService
{
  GObject parent_instance;

  sqlite3 *db;
};

G_DEFINE_FINAL_TYPE (GigHistoryService, gig_history_service, G_TYPE_OBJECT)

static const gchar *migrations[] = {
  "/com/github/obyknovenius/Gig/history/001-create-tables.sql",
};

static void
gig_history_service_set_error (GigHistoryService *self,
                               GError **error,
                               const gchar *what)
{
  g_assert (GIG_IS_HISTORY_SERVICE (self));

  g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
               "%s: %s", what, sqlite3_errmsg (self->db));
}

static gboolean
gig_history_service_exec (GigHistoryService *self,
                          const gchar *sql,
                          GError **error)
{
  gchar *errmsg = NULL;

  g_assert (GIG_IS_HISTORY_SERVICE (self));

  if (sqlite3_exec (self->db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s", errmsg);
      sqlite3_free (errmsg);
      return FALSE;
    }

  return TRUE;
}

static gint
gig_history_service_get_user_version (GigHistoryService *self,
                                      GError **error)
{
  sqlite3_stmt *stmt = NULL;
  gint version = -1;

  g_assert (GIG_IS_HISTORY_SERVICE (self));

  if (sqlite3_prepare_v2 (self->db, "PRAGMA user_version", -1, &stmt, NULL) != SQLITE_OK)
    {
      gig_history_service_set_error (self, error, "Could not read schema version");
      return -1;
    }

  if (sqlite3_step (stmt) == SQLITE_ROW)
    version = sqlite3_column_int (stmt, 0);
  else
    gig_history_service_set_error (self, error, "Could not read schema version");

  sqlite3_finalize (stmt);

  return version;
}

static gboolean
gig_history_service_apply_migration (GigHistoryService *self,
                                     gint version,
                                     GError **error)
{
  g_autoptr (GBytes) bytes = NULL;
  g_autofree gchar *set_version = NULL;
  const gchar *resource_path;

  g_assert (GIG_IS_HISTORY_SERVICE (self));
  g_assert (version >= 1 && version <= (gint) G_N_ELEMENTS (migrations));

  resource_path = migrations[version - 1];

  bytes = g_resources_lookup_data (resource_path, G_RESOURCE_LOOKUP_FLAGS_NONE, error);
  if (!bytes)
    return FALSE;

  set_version = g_strdup_printf ("PRAGMA user_version = %d", version);

  if (!gig_history_service_exec (self, "BEGIN", error))
    return FALSE;

  if (!gig_history_service_exec (self, g_bytes_get_data (bytes, NULL), error) ||
      !gig_history_service_exec (self, set_version, error) ||
      !gig_history_service_exec (self, "COMMIT", error))
    {
      gig_history_service_exec (self, "ROLLBACK", NULL);
      g_prefix_error (error, "Could not apply %s: ", resource_path);
      return FALSE;
    }

  return TRUE;
}

static gboolean
gig_history_service_migrate (GigHistoryService *self,
                             GError **error)
{
  gint version;
  gint latest = G_N_ELEMENTS (migrations);

  g_assert (GIG_IS_HISTORY_SERVICE (self));

  if ((version = gig_history_service_get_user_version (self, error)) < 0)
    return FALSE;

  if (version > latest)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Database schema version %d is newer than supported version %d",
                   version, latest);
      return FALSE;
    }

  for (gint i = version + 1; i <= latest; i++)
    if (!gig_history_service_apply_migration (self, i, error))
      return FALSE;

  return TRUE;
}

static gboolean
gig_history_service_open (GigHistoryService *self,
                          const gchar *path,
                          GError **error)
{
  g_assert (GIG_IS_HISTORY_SERVICE (self));

  if (sqlite3_open_v2 (path, &self->db,
                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                       NULL) != SQLITE_OK)
    {
      gig_history_service_set_error (self, error, "Could not open history database");
      return FALSE;
    }

  if (!gig_history_service_exec (self,
                                 "PRAGMA journal_mode = WAL;"
                                 "PRAGMA synchronous = NORMAL;"
                                 "PRAGMA foreign_keys = ON;",
                                 error))
    return FALSE;

  return gig_history_service_migrate (self, error);
}

static void
gig_history_service_finalize (GObject *object)
{
  GigHistoryService *self = (GigHistoryService *) object;

  g_assert (GIG_IS_HISTORY_SERVICE (self));

  g_clear_pointer (&self->db, sqlite3_close);

  G_OBJECT_CLASS (gig_history_service_parent_class)->finalize (object);
}

static void
gig_history_service_class_init (GigHistoryServiceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gig_history_service_finalize;
}

static void
gig_history_service_init (GigHistoryService *self)
{
}

GigHistoryService *
gig_history_service_new (const gchar *path,
                         GError **error)
{
  g_autoptr (GigHistoryService) self = NULL;

  g_return_val_if_fail (path != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  self = g_object_new (GIG_TYPE_HISTORY_SERVICE, NULL);

  if (!gig_history_service_open (self, path, error))
    return NULL;

  return g_steal_pointer (&self);
}
