#include "gig-history-service.h"

#include "gig-history-visit.h"

#include <sqlite3.h>

struct _GigHistoryService
{
  GObject parent_instance;

  sqlite3 *db;
};

G_DEFINE_FINAL_TYPE (GigHistoryService, gig_history_service, G_TYPE_OBJECT)

G_DEFINE_AUTOPTR_CLEANUP_FUNC (sqlite3_stmt, sqlite3_finalize)

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

static sqlite3_stmt *
gig_history_service_prepare (GigHistoryService *self,
                             const gchar *sql,
                             GError **error)
{
  sqlite3_stmt *stmt = NULL;

  g_assert (GIG_IS_HISTORY_SERVICE (self));

  if (sqlite3_prepare_v2 (self->db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
      gig_history_service_set_error (self, error, "Could not prepare statement");
      return NULL;
    }

  return stmt;
}

static gint
gig_history_service_get_user_version (GigHistoryService *self,
                                      GError **error)
{
  g_autoptr (sqlite3_stmt) stmt = NULL;

  g_assert (GIG_IS_HISTORY_SERVICE (self));

  stmt = gig_history_service_prepare (self, "PRAGMA user_version", error);
  if (!stmt)
    return -1;

  if (sqlite3_step (stmt) != SQLITE_ROW)
    {
      gig_history_service_set_error (self, error, "Could not read schema version");
      return -1;
    }

  return sqlite3_column_int (stmt, 0);
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

static gboolean
should_record (const gchar *uri)
{
  const gchar *scheme;

  g_assert (uri != NULL);

  scheme = g_uri_peek_scheme (uri);
  if (!scheme)
    return FALSE;

  /* TODO: Decide whether to record file:// URIs; Epiphany does. */
  return g_str_equal (scheme, "http") || g_str_equal (scheme, "https");
}

/* Returns TRUE without writing anything if @uri is not one that history
 * records, such as about:blank. */
gboolean
gig_history_service_add_visit (GigHistoryService *self,
                               const gchar *uri,
                               GError **error)
{
  g_autoptr (sqlite3_stmt) upsert_page = NULL;
  g_autoptr (sqlite3_stmt) insert_visit = NULL;
  gint64 now = g_get_real_time ();
  sqlite3_int64 page_id;

  g_return_val_if_fail (GIG_IS_HISTORY_SERVICE (self), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (!should_record (uri))
    return TRUE;

  /* The no-op update makes RETURNING give the id of an existing page too.
   * DO NOTHING would return no row for it. */
  upsert_page = gig_history_service_prepare (self,
                                             "INSERT INTO pages (uri) VALUES (?) "
                                             "ON CONFLICT (uri) DO UPDATE SET uri = excluded.uri "
                                             "RETURNING id",
                                             error);
  if (!upsert_page)
    return FALSE;

  insert_visit = gig_history_service_prepare (self,
                                              "INSERT INTO visits (page_id, time) VALUES (?, ?)",
                                              error);
  if (!insert_visit)
    return FALSE;

  if (!gig_history_service_exec (self, "BEGIN", error))
    return FALSE;

  sqlite3_bind_text (upsert_page, 1, uri, -1, SQLITE_STATIC);

  if (sqlite3_step (upsert_page) != SQLITE_ROW)
    {
      gig_history_service_set_error (self, error, "Could not update page");
      goto rollback;
    }

  page_id = sqlite3_column_int64 (upsert_page, 0);
  sqlite3_reset (upsert_page);

  sqlite3_bind_int64 (insert_visit, 1, page_id);
  sqlite3_bind_int64 (insert_visit, 2, now);

  if (sqlite3_step (insert_visit) != SQLITE_DONE)
    {
      gig_history_service_set_error (self, error, "Could not insert visit");
      goto rollback;
    }

  if (!gig_history_service_exec (self, "COMMIT", error))
    goto rollback;

  return TRUE;

rollback:
  gig_history_service_exec (self, "ROLLBACK", NULL);
  return FALSE;
}

/* Returns TRUE without writing anything if @title is NULL or empty, so a
 * page that briefly reports no title keeps the one already stored. The
 * scheme check is not needed: pages only holds URIs that add_visit records. */
gboolean
gig_history_service_set_title (GigHistoryService *self,
                               const gchar *uri,
                               const gchar *title,
                               GError **error)
{
  g_autoptr (sqlite3_stmt) stmt = NULL;

  g_return_val_if_fail (GIG_IS_HISTORY_SERVICE (self), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (!uri || !title || title[0] == '\0')
    return TRUE;

  stmt = gig_history_service_prepare (self,
                                      "UPDATE pages SET title = ? WHERE uri = ?",
                                      error);
  if (!stmt)
    return FALSE;

  sqlite3_bind_text (stmt, 1, title, -1, SQLITE_STATIC);
  sqlite3_bind_text (stmt, 2, uri, -1, SQLITE_STATIC);

  if (sqlite3_step (stmt) != SQLITE_DONE)
    {
      gig_history_service_set_error (self, error, "Could not update title");
      return FALSE;
    }

  return TRUE;
}

GListModel *
gig_history_service_find_visits (GigHistoryService *self,
                                 const gchar *search,
                                 guint limit,
                                 GError **error)
{
  g_autoptr (sqlite3_stmt) stmt = NULL;
  g_autoptr (GListStore) visits = NULL;
  gint rc;

  g_return_val_if_fail (GIG_IS_HISTORY_SERVICE (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  /* ?1 appears three times, so it is numbered rather than a plain ?.
   * A NULL search means "no search" and matches every visit. instr() does a
   * plain substring match, so no characters in the search need escaping.
   * lower() only folds ASCII, the same as LIKE's case-insensitivity. */
  stmt = gig_history_service_prepare (self,
                                      "SELECT pages.uri, pages.title, visits.time "
                                      "FROM visits JOIN pages ON pages.id = visits.page_id "
                                      "WHERE ?1 IS NULL "
                                      "   OR instr (lower (pages.uri), lower (?1)) > 0 "
                                      "   OR instr (lower (pages.title), lower (?1)) > 0 "
                                      "ORDER BY visits.time DESC "
                                      "LIMIT ?2",
                                      error);
  if (!stmt)
    return NULL;

  if (search && search[0] == '\0')
    search = NULL;

  sqlite3_bind_text (stmt, 1, search, -1, SQLITE_STATIC);
  sqlite3_bind_int64 (stmt, 2, limit);

  visits = g_list_store_new (GIG_TYPE_HISTORY_VISIT);

  while ((rc = sqlite3_step (stmt)) == SQLITE_ROW)
    {
      g_autoptr (GigHistoryVisit) visit = NULL;

      visit = gig_history_visit_new ((const gchar *) sqlite3_column_text (stmt, 0),
                                     (const gchar *) sqlite3_column_text (stmt, 1),
                                     sqlite3_column_int64 (stmt, 2));

      g_list_store_append (visits, visit);
    }

  if (rc != SQLITE_DONE)
    {
      gig_history_service_set_error (self, error, "Could not query visits");
      return NULL;
    }

  return G_LIST_MODEL (g_steal_pointer (&visits));
}
