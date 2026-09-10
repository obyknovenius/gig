#include "gig-utils.h"

#include <libsoup/soup.h>
#include <string.h>

static gboolean
host_is_plausible (const gchar *host)
{
  if (g_hostname_is_ip_address (host))
    return TRUE;

  if (g_strcmp0 (host, "localhost") == 0)
    return TRUE;

  if (soup_tld_get_base_domain (host, NULL))
    return TRUE;

  return FALSE;
}

gchar *
gig_utils_fixup_uri (const gchar *uri)
{
  const gchar *scheme;
  g_autofree gchar *uri_with_scheme = NULL;
  g_autoptr (GUri) parsed_uri = NULL;
  const gchar *host = NULL;

  g_return_val_if_fail (uri != NULL && uri[0] != '\0', NULL);

  scheme = g_uri_peek_scheme (uri);
  uri_with_scheme = scheme ? g_strdup (uri) : g_strconcat ("https://", uri, NULL);

  parsed_uri = g_uri_parse (uri_with_scheme, G_URI_FLAGS_NONE, NULL);
  if (!parsed_uri)
    return NULL;

  host = g_uri_get_host (parsed_uri);
  if (!host)
    return NULL;

  if (!scheme && !host_is_plausible (host))
    return NULL;

  return g_steal_pointer (&uri_with_scheme);
}

gchar *
gig_utils_build_search_uri (const gchar *query)
{
  g_autofree gchar *escaped_query = NULL;

  g_return_val_if_fail (query != NULL && query[0] != '\0', NULL);

  escaped_query = g_uri_escape_string (query, NULL, FALSE);

  return g_strconcat ("https://duckduckgo.com/?q=", escaped_query, NULL);
}

gchar *
gig_utils_get_base_domain (const gchar *uri,
                           guint *start_index,
                           guint *end_index)
{
  g_autoptr (GUri) parsed_uri = NULL;
  const gchar *host = NULL;
  const gchar *base_domain = NULL;

  g_return_val_if_fail (uri != NULL && uri[0] != '\0', NULL);

  parsed_uri = g_uri_parse (uri, G_URI_FLAGS_NONE, NULL);
  if (!parsed_uri)
    return NULL;

  host = g_uri_get_host (parsed_uri);
  if (!host)
    return NULL;

  base_domain = soup_tld_get_base_domain (host, NULL);
  if (!base_domain)
    return NULL;

  if (start_index || end_index)
    {
      const gchar *substring = g_strstr_len (uri, -1, base_domain);
      g_assert (substring != NULL);

      if (start_index)
        *start_index = substring - uri;

      if (end_index)
        *end_index = (substring - uri) + strlen (base_domain);
    }

  return g_strdup (base_domain);
}
