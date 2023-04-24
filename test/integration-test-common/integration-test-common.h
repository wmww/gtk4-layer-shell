#ifndef TEST_CLIENT_COMMON_H
#define TEST_CLIENT_COMMON_H

#include "gtk4-layer-shell.h"
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
#define DONT_EXPECT_MESSAGE(message) fprintf(stderr, "DONT_EXPECT: %s\n", #message)
// Tell the test script that all expected messages should now be fulfilled
// (called automatically before each callback and at the end of the test)
#define CHECK_EXPECTATIONS() fprintf(stderr, "CHECK EXPECTATIONS COMPLETED\n")

// NULL-terminated list of callbacks that will be called before quitting
// Should be defined in the test file using TEST_CALLBACKS()
extern void (* test_callbacks[])(void);

// Input is a sequence of callback names with a trailing comma
#define TEST_CALLBACKS(...) void (* test_callbacks[])(void) = {__VA_ARGS__ NULL};

GtkWindow* create_default_window();

#endif // TEST_CLIENT_COMMON_H
