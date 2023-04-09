#include "gtk-layer-demo.h"

static void
quit_activated(GSimpleAction *action, GVariant *parameter, GApplication *application) {
    (void)action; (void)parameter;
    g_application_quit (application);
}

void
set_up_menubar (GtkWindow *layer_window)
{
    GSimpleAction *act_quit = g_simple_action_new ("quit", NULL);
    g_action_map_add_action (G_ACTION_MAP (gtk_window_get_application (layer_window)), G_ACTION (act_quit));
    g_signal_connect (act_quit, "activate", G_CALLBACK (quit_activated), gtk_window_get_application (layer_window));

    GMenu *menubar = g_menu_new ();
    gtk_application_set_menubar (GTK_APPLICATION (gtk_window_get_application (layer_window)), G_MENU_MODEL (menubar));
    gtk_application_window_set_show_menubar (GTK_APPLICATION_WINDOW (layer_window), TRUE);
    GMenuItem *menu_item = g_menu_item_new ("GTK4 Layer Demo", NULL);
    g_menu_append_item (menubar, menu_item);
    GMenu *menu = g_menu_new ();
    GMenuItem *menu_item_quit = g_menu_item_new ("Quit", "app.quit");
    g_menu_append_item (menu, menu_item_quit);
    g_menu_item_set_submenu (menu_item, G_MENU_MODEL (menu));
    g_menu_append_item (menubar, menu_item);
    g_object_unref (menu_item_quit);
    g_object_unref (menu_item);
}
