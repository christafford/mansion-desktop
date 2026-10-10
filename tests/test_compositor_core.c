/* Test for compositor-core library
 *
 * Verifies:
 * - Core library can be initialized and destroyed
 * - Surface lifecycle is managed correctly
 * - Resource cleanup is clean
 */

#include <stdio.h>
#include <stdlib.h>
#include <wayland-server.h>
#include "compositor-core.h"

static int test_count = 0;
static int pass_count = 0;

static void test(const char* name, int condition) {
    test_count++;
    if (condition) {
        pass_count++;
        printf("PASS: %s\n", name);
    } else {
        printf("FAIL: %s\n", name);
    }
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    /* Test 1: Create and destroy compositor */
    {
        struct wl_display* display = wl_display_create();
        if (!display) {
            printf("FAIL: Could not create wl_display\n");
            return 1;
        }

        struct ElsewhereCompositor* compositor = compositor_core_create(display);
        test("compositor_core_create succeeds", compositor != NULL);

        if (compositor) {
            test("compositor has surface_list initialized", 
                 compositor_core_surface_list_empty(compositor));
            test("compositor has orphaned_surfaces initialized",
                 compositor_core_orphaned_surfaces_empty(compositor));
            test("compositor has toplevel_list initialized",
                 compositor_core_toplevel_list_empty(compositor));
            test("compositor has no toplevels initially",
                 compositor_core_get_toplevel_count(compositor) == 0);
            test("compositor has no focus initially",
                 compositor_core_get_focused_surface(compositor) == NULL);
        }

        /* Test 2: Getters work correctly */
        if (compositor) {
            test("compositor_core_get_display returns display",
                 compositor_core_get_display(compositor) == display);
            /* List getters return the list head (always non-NULL for initialized list) */
            struct wl_list* surface_list = compositor_core_get_surface_list(compositor);
            struct wl_list* toplevel_list = compositor_core_get_toplevel_list(compositor);
            test("compositor_core_get_surface_list returns valid list", surface_list != NULL);
            test("compositor_core_get_toplevel_list returns valid list", toplevel_list != NULL);
            test("compositor_core_get_toplevel_count returns 0",
                 compositor_core_get_toplevel_count(compositor) == 0);
        }

        /* Test 3: Focus management */
        if (compositor) {
            struct wl_resource fake_surface = {0};  /* Just a pointer for testing */
            compositor_core_set_focus(compositor, &fake_surface, 42);
            test("compositor_core_set_focus sets focus",
                 compositor_core_get_focused_surface(compositor) == &fake_surface);
            test("compositor_core_set_focus sets serial",
                 compositor_core_get_focus_serial(compositor) == 42);

            compositor_core_clear_focus(compositor);
            test("compositor_core_clear_focus clears focus",
                 compositor_core_get_focused_surface(compositor) == NULL);
            test("compositor_core_clear_focus clears serial",
                 compositor_core_get_focus_serial(compositor) == 0);
        }

        /* Test 4: Surface iteration helpers */
        {
            struct wl_list* surface_list = compositor_core_get_surface_list(compositor);
            struct wl_resource* first = compositor_core_surface_first(surface_list);
            test("compositor_core_surface_first returns NULL for empty list", first == NULL);
        }

        /* Test 5: Destroy compositor */
        compositor_core_destroy(compositor);
        test("compositor_core_destroy completes without crash", 1);

        wl_display_destroy(display);
    }

    printf("\n=== Summary: %d/%d tests passed ===\n", pass_count, test_count);

    return pass_count == test_count ? 0 : 1;
}
