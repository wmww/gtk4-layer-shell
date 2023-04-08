#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define DEFAULT_OUTPUT_WIDTH 1920
#define DEFAULT_OUTPUT_HEIGHT 1080

#define FATAL_FMT(format, ...) do {fprintf(stderr, "Fatal error at %s:%d in %s(): " format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); exit(1);} while (0)
#define FATAL(message) FATAL_FMT(message"%s", "")
#define ASSERT(assertion) do {if (!(assertion)) {FATAL_FMT("\n  assertion failed: %s", #assertion);}} while (0)
#define ASSERT_EQ(a, b, format) do {if (!((a) == (b))) {FATAL_FMT("\n  expected: %s == %s\n  actual:   " format " != " format "\n", #a, #b, a, b);}} while (0)
#define ASSERT_STR_EQ(a, b) do {if (strcmp(a, b)) {FATAL_FMT("\n  expected: %s ≈ %s\n  actual:   \"%s\" ≠ \"%s\"\n", #a, #b, a, b);}} while (0)

#endif // TEST_COMMON_H

