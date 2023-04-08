#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    // The mock server will automatically click on our window, triggering the menu to open

    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface);
    EXPECT_MESSAGE(xdg_wm_base .get_xdg_surface);
    EXPECT_MESSAGE(xdg_surface .get_popup);
    EXPECT_MESSAGE(xdg_popup .grab);

    window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    GtkWidget *menu_bar = gtk_menu_bar_new();
    gtk_container_add(GTK_CONTAINER(window), menu_bar);
    GtkWidget *menu_item = gtk_menu_item_new_with_label("Popup menu");
    gtk_container_add(GTK_CONTAINER(menu_bar), menu_item);
    GtkWidget *submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), submenu);
    GtkWidget *close_item = gtk_menu_item_new_with_label("Menu item");
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), close_item);

    gtk_layer_init_for_window(window);
    gtk_widget_show_all(GTK_WIDGET(window));
}

TEST_CALLBACKS(
    callback_0,
)
