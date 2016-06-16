#include <gtk/gtk.h>

static void
set_ui_param (GtkWidget *widget,
              gpointer   data)
{
  *((double*) data) = gtk_range_get_value(GTK_RANGE(widget));
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
  GtkWidget *window;
  GtkWidget *grid;
  GtkWidget *scale;

  /* create a new window, and set its title */
  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Klugesynth");
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);

  /* Here we construct the container that is going pack our buttons */
  grid = gtk_grid_new ();

  /* Pack the container in the window */
  gtk_container_add (GTK_CONTAINER (window), grid);

  for (int i = 0; i < 10; i++) {
    scale = gtk_scale_new_with_range (GTK_ORIENTATION_VERTICAL, 0, 1, 0.01);
    gtk_widget_set_size_request(scale, 50, 500);
    g_signal_connect (scale, "value_changed", G_CALLBACK (set_ui_param), ui_params + i);
    gtk_grid_attach (GTK_GRID (grid), scale, i, 0, 1, 1);
  }
  /* Now that we are done packing our widgets, we show them all
   * in one go, by calling gtk_widget_show_all() on the window.
   * This call recursively calls gtk_widget_show() on all widgets
   * that are contained in the window, directly or indirectly.
   */
  gtk_widget_show_all (window);
}

int
ui_main (int    argc,
         char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("com.github.frostburn.kludgesynth", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
