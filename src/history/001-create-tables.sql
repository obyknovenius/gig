-- One row per distinct URL, with aggregated stats for autocompletion.
CREATE TABLE urls (
  id              INTEGER PRIMARY KEY,
  url             TEXT    NOT NULL UNIQUE,
  title           TEXT,
  visit_count     INTEGER NOT NULL DEFAULT 0,
  typed_count     INTEGER NOT NULL DEFAULT 0,  -- visits typed into the address bar
  last_visit_time INTEGER NOT NULL             -- g_get_real_time (), µs since epoch
);

-- One row per page load, for the "by day" view and time-based deletion.
CREATE TABLE visits (
  id     INTEGER PRIMARY KEY,
  url_id INTEGER NOT NULL REFERENCES urls (id) ON DELETE CASCADE,
  time   INTEGER NOT NULL,                     -- g_get_real_time (), µs since epoch
  typed  INTEGER NOT NULL DEFAULT 0            -- 1 if typed into the address bar
);

CREATE INDEX visits_time ON visits (time);

-- Without this, every delete from urls scans visits to cascade.
CREATE INDEX visits_url_id ON visits (url_id);
