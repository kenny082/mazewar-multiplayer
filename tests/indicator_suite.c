#include <criterion/criterion.h>
#include <stdio.h>

#include "excludes.h"

/*
 * "Tests" that just assign one point for each module for which an
 * implementation has been provided.
 */

Test(indicator_suite, client_registry, .timeout = 5) {
#ifdef NO_CLIENT_REGISTRY
    cr_assert_fail("The client registry module was not implemented");
#endif
}

Test(indicator_suite, main, .timeout = 5) {
#ifdef NO_MAIN
    cr_assert_fail("The main module was not implemented");
#endif
}
Test(indicator_suite, maze, .timeout = 5) {
#ifdef NO_MAZE
    cr_assert_fail("The maze module was not implemented");
#endif
}
Test(indicator_suite, player, .timeout = 5) {
#ifdef NO_PLAYER
    cr_assert_fail("The player module was not implemented");
#endif
}
Test(indicator_suite, protocol, .timeout = 5) {
#ifdef NO_PROTOCOL
    cr_assert_fail("The protocol module was not implemented");
#endif
}
Test(indicator_suite, server, .timeout = 5) {
#ifdef NO_SERVER
    cr_assert_fail("The server module was not implemented");
#endif
}
