#ifndef TEST_CLIENT_COMMON_H
#define TEST_CLIENT_COMMON_H

#include "gtk4-layer-shell.h"
#include "gtk4-session-lock.h"
#include "test-common.h"
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/wayland/gdkwayland.h>
#include <stdio.h>

// Time in milliseconds for each callback to run
extern int step_time;

// Tell the test script that a request containing the given space-separated components is expected
#define EXPECT_MESSAGE(message) fprintf(stderr, "EXPECT: %s\n", #message)
// Tell the test script this request is not expected
#define UNEXPECT_MESSAGE(message) fprintf(stderr, "UNEXPECT: %s\n", #message)
// Tell the test script that all expected messages should now be fulfilled
// (called automatically before each callback and at the end of the test)
#define CHECK_EXPECTATIONS() fprintf(stderr, "CHECK EXPECTATIONS COMPLETED\n")

// NULL-terminated list of callbacks that will be called before quitting
// Should be defined in the test file using TEST_CALLBACKS()
extern void (* test_callbacks[])(void);

// Input is a sequence of callback names with a trailing comma
#define TEST_CALLBACKS(...) void (* test_callbacks[])(void) = {__VA_ARGS__ NULL};

// Send a command to the mock server and verify the response is correct
void send_command(const char* command, const char* expected_response);

GtkWindow* create_default_window();

GtkWidget* popup_widget_new();
void popup_widget_toggle_open(GtkWidget* widget);

enum lock_state_t {
    LOCK_STATE_UNLOCKED = 0,
    LOCK_STATE_LOCKED,
    LOCK_STATE_FAILED,
};
void connect_lock_signals_except_monitor(GtkSessionLockInstance* lock, enum lock_state_t* state);
void connect_lock_signals(GtkSessionLockInstance* lock, enum lock_state_t* state);

#endif // TEST_CLIENT_COMMON_H
