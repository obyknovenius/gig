#include "config.h"

#include "gig-settings.h"

struct _GigSettings
{
  GObject parent_instance;

  GSettings *settings;
};

G_DEFINE_FINAL_TYPE (GigSettings, gig_settings, G_TYPE_OBJECT)

static void
gig_settings_finalize (GObject *object)
{
  GigSettings *self = GIG_SETTINGS (object);

  g_clear_object (&self->settings);

  G_OBJECT_CLASS (gig_settings_parent_class)->finalize (object);
}

static void
gig_settings_class_init (GigSettingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gig_settings_finalize;
}

static void
gig_settings_init (GigSettings *self)
{
  self->settings = g_settings_new (APP_SCHEMA_ID);
}

GigSettings *
gig_settings_new (void)
{
  return g_object_new (GIG_TYPE_SETTINGS, NULL);
}

void
gig_settings_get_default_window_size (GigSettings *self,
                                      gint *width,
                                      gint *height)
{
  g_return_if_fail (GIG_IS_SETTINGS (self));

  g_settings_get (self->settings, "default-window-size", "(ii)", width, height);
}

void
gig_settings_set_default_window_size (GigSettings *self,
                                      gint width,
                                      gint height)
{
  g_return_if_fail (GIG_IS_SETTINGS (self));

  g_settings_set (self->settings, "default-window-size", "(ii)", width, height);
}
