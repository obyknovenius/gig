#include "gig-history-dialog.h"

#include "gig-history-service.h"
#include "gig-history-visit.h"

/* TODO: Load more rows when scrolling to the end, like Epiphany does. */
#define MAX_VISITS 200

struct _GigHistoryDialog
{
  AdwDialog parent_instance;

  GtkSearchBar *search_bar;
  GtkSearchEntry *search_entry;
  GtkStack *stack;
  GtkBox *days_box;

  GigHistoryService *history_service;
};

G_DEFINE_FINAL_TYPE (GigHistoryDialog, gig_history_dialog, ADW_TYPE_DIALOG)

enum
{
  PROP_0,
  PROP_HISTORY_SERVICE,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static GDate *
get_local_date (GDateTime *date_time)
{
  return g_date_new_dmy (g_date_time_get_day_of_month (date_time),
                         g_date_time_get_month (date_time),
                         g_date_time_get_year (date_time));
}

static gchar *
format_day (GDateTime *date_time)
{
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();
  g_autoptr (GDate) today = get_local_date (now);
  g_autoptr (GDate) day = get_local_date (date_time);
  gint days_ago = g_date_days_between (day, today);

  if (days_ago == 0)
    return g_strdup ("Today");
  if (days_ago == 1)
    return g_strdup ("Yesterday");
  if (g_date_time_get_year (date_time) == g_date_time_get_year (now))
    return g_date_time_format (date_time, "%A, %B %-d");

  return g_date_time_format (date_time, "%A, %B %-d, %Y");
}

static GtkWidget *
gig_history_dialog_create_row (GigHistoryDialog *self,
                               GigHistoryVisit *visit,
                               GDateTime *date_time)
{
  GtkWidget *row;
  GtkWidget *time_label;
  const gchar *title;
  const gchar *uri;
  g_autofree gchar *time = g_date_time_format (date_time, "%R");

  g_assert (GIG_IS_HISTORY_DIALOG (self));

  title = gig_history_visit_get_title (visit);
  uri = gig_history_visit_get_uri (visit);

  row = adw_action_row_new ();
  adw_preferences_row_set_use_markup (ADW_PREFERENCES_ROW (row), FALSE);
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), title ? title : uri);
  adw_action_row_set_subtitle (ADW_ACTION_ROW (row), uri);
  adw_action_row_set_title_lines (ADW_ACTION_ROW (row), 1);
  adw_action_row_set_subtitle_lines (ADW_ACTION_ROW (row), 1);

  time_label = gtk_label_new (time);
  gtk_widget_add_css_class (time_label, "dimmed");
  adw_action_row_add_suffix (ADW_ACTION_ROW (row), time_label);

  return row;
}

static void
gig_history_dialog_reload (GigHistoryDialog *self)
{
  g_autoptr (GListModel) visits = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (GDate) group_day = NULL;
  AdwPreferencesGroup *group = NULL;
  GtkWidget *child;
  const gchar *search;
  guint n_visits = 0;

  g_assert (GIG_IS_HISTORY_DIALOG (self));
  g_assert (GIG_IS_HISTORY_SERVICE (self->history_service));

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self->days_box))))
    gtk_box_remove (self->days_box, child);

  search = gtk_editable_get_text (GTK_EDITABLE (self->search_entry));

  /* TODO: Query on the worker thread once the service has one. */
  visits = gig_history_service_find_visits (self->history_service, search, MAX_VISITS, &error);
  if (visits)
    n_visits = g_list_model_get_n_items (visits);
  else
    g_warning ("Could not load history: %s", error->message);

  /* Visits come newest first, so a new group starts whenever the day changes. */
  for (guint i = 0; i < n_visits; i++)
    {
      g_autoptr (GigHistoryVisit) visit = g_list_model_get_item (visits, i);
      g_autoptr (GDateTime) date_time = g_date_time_new_from_unix_local_usec (gig_history_visit_get_time (visit));
      g_autoptr (GDate) day = get_local_date (date_time);

      if (!group_day || g_date_compare (day, group_day) != 0)
        {
          g_autofree gchar *title = format_day (date_time);

          group = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
          adw_preferences_group_set_title (group, title);
          gtk_box_append (self->days_box, GTK_WIDGET (group));

          g_clear_pointer (&group_day, g_date_free);
          group_day = g_steal_pointer (&day);
        }

      adw_preferences_group_add (group, gig_history_dialog_create_row (self, visit, date_time));
    }

  if (n_visits > 0)
    gtk_stack_set_visible_child_name (self->stack, "results");
  else if (search[0] != '\0')
    gtk_stack_set_visible_child_name (self->stack, "no-results");
  else
    gtk_stack_set_visible_child_name (self->stack, "empty");
}

static void
gig_history_dialog_search_entry_search_changed_cb (GigHistoryDialog *self,
                                                   GtkSearchEntry *search_entry)
{
  g_assert (GIG_IS_HISTORY_DIALOG (self));

  gig_history_dialog_reload (self);
}

static void
gig_history_dialog_search_action (GtkWidget *widget,
                                  const gchar *action_name,
                                  GVariant *param)
{
  GigHistoryDialog *self = (GigHistoryDialog *) widget;

  g_assert (GIG_IS_HISTORY_DIALOG (self));

  gtk_search_bar_set_search_mode (self->search_bar, TRUE);
  gtk_widget_grab_focus (GTK_WIDGET (self->search_entry));
}

static void
gig_history_dialog_constructed (GObject *object)
{
  GigHistoryDialog *self = (GigHistoryDialog *) object;

  g_assert (GIG_IS_HISTORY_DIALOG (self));

  G_OBJECT_CLASS (gig_history_dialog_parent_class)->constructed (object);

  gig_history_dialog_reload (self);
}

static void
gig_history_dialog_dispose (GObject *object)
{
  GigHistoryDialog *self = (GigHistoryDialog *) object;

  g_assert (GIG_IS_HISTORY_DIALOG (self));

  gtk_widget_dispose_template (GTK_WIDGET (self), GIG_TYPE_HISTORY_DIALOG);

  g_clear_object (&self->history_service);

  G_OBJECT_CLASS (gig_history_dialog_parent_class)->dispose (object);
}

static void
gig_history_dialog_set_property (GObject *object,
                                 guint prop_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
  GigHistoryDialog *self = (GigHistoryDialog *) object;

  g_assert (GIG_IS_HISTORY_DIALOG (self));

  switch (prop_id)
    {
    case PROP_HISTORY_SERVICE:
      g_set_object (&self->history_service, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gig_history_dialog_class_init (GigHistoryDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = gig_history_dialog_constructed;
  object_class->dispose = gig_history_dialog_dispose;
  object_class->set_property = gig_history_dialog_set_property;

  properties[PROP_HISTORY_SERVICE] =
      g_param_spec_object ("history-service",
                           NULL, NULL,
                           GIG_TYPE_HISTORY_SERVICE,
                           G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/obyknovenius/Gig/history/gig-history-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, GigHistoryDialog, search_bar);
  gtk_widget_class_bind_template_child (widget_class, GigHistoryDialog, search_entry);
  gtk_widget_class_bind_template_child (widget_class, GigHistoryDialog, stack);
  gtk_widget_class_bind_template_child (widget_class, GigHistoryDialog, days_box);

  gtk_widget_class_bind_template_callback (widget_class, gig_history_dialog_search_entry_search_changed_cb);

  gtk_widget_class_install_action (widget_class, "history.search", NULL, gig_history_dialog_search_action);

  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_f, GDK_CONTROL_MASK, "history.search", NULL);
}

static void
gig_history_dialog_init (GigHistoryDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_search_bar_connect_entry (self->search_bar, GTK_EDITABLE (self->search_entry));
}

GigHistoryDialog *
gig_history_dialog_new (GigHistoryService *history_service)
{
  g_return_val_if_fail (GIG_IS_HISTORY_SERVICE (history_service), NULL);

  return g_object_new (GIG_TYPE_HISTORY_DIALOG,
                       "history-service", history_service,
                       NULL);
}
