-- One row per distinct page.
CREATE TABLE pages (
  id    INTEGER PRIMARY KEY,
  uri   TEXT    NOT NULL UNIQUE,
  title TEXT
);

-- One row per page load, for the "by day" view and time-based deletion.
CREATE TABLE visits (
  id      INTEGER PRIMARY KEY,
  page_id INTEGER NOT NULL REFERENCES pages (id) ON DELETE CASCADE,
  time    INTEGER NOT NULL                     -- g_get_real_time (), µs since epoch
);

CREATE INDEX visits_time ON visits (time);

-- Without this, every delete from pages scans visits to cascade.
CREATE INDEX visits_page_id ON visits (page_id);
