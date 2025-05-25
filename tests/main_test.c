#include <criterion/criterion.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

/*
 * Tests of argument processing by main() and the ability to load a maze template
 * and fail gracefully on ill-formed templates.
 */

/*
 * We invoke the server for these tests under different names, so that we don't
 * end up killing the wrong processes.
 */

void link_exec(void) {
  link("bin/mazewar", "bin/mzw1");
  link("bin/mazewar", "bin/mzw2");
  link("bin/mazewar", "bin/mzw3");
  link("bin/mazewar", "bin/mzw4");
  link("bin/mazewar", "bin/mzw5");
  link("bin/mazewar", "bin/mzw6");
  link("bin/mazewar", "bin/mzw7");
  link("bin/mazewar", "bin/mzw8");
  link("bin/mazewar", "bin/mzw9");
}

/*
 * Invoke the server with no arguments.  Should exit with EXIT_FAILURE.
 */
Test(main_suite, no_args, .timeout = 5, .init = link_exec) {
#ifdef NO_MAIN
    cr_assert_fail("The main module was not implemented");
#endif
    int ret = system("bin/mzw1");
    cr_assert(WIFEXITED(ret), "The program did not exit normally.");
    int exp = EXIT_FAILURE;
    cr_assert_eq(WEXITSTATUS(ret), exp, "The program exited with status %d (%d expected)",
		 WEXITSTATUS(ret), exp);
}

/*
 * Invoke the server with just a port argument and a timeout.
 * The server should exit with the specified timeout signal, not some other way.
 */
Test(main_suite, port_arg_only, .timeout = 5, .init = link_exec) {
#ifdef NO_MAIN
    cr_assert_fail("The main module was not implemented");
#endif
    int ret;
    ret = system("(sleep 1; killall -q -USR2 bin/mzw2) & bin/mzw2 -p 9001");
    // The return value from system() is always an exit status.
    // However, exits with a signal seem to get folded onto the low eight bits.
    cr_assert(WEXITSTATUS(ret) & 0x80, "The program did not exit with a signal.");
    int exp = SIGUSR2;
    cr_assert_eq(WEXITSTATUS(ret) & 0x7f, exp, "The program exited with signal %d (%d expected)",
		 WEXITSTATUS(ret) & 0x7f, exp);
}

/*
 * Invoke the server with a port argument and a good template argument.
 * The server should exit with the specified timeout signal, not some other way.
 */
Test(main_suite, good_template, .timeout = 5, .init = link_exec) {
#ifdef NO_MAIN
    cr_assert_fail("The main module was not implemented");
#endif
    int ret;
    ret = system("(sleep 1; killall -q -USR2 bin/mzw3) & bin/mzw3 -p 9002 -t tests/rsrc/good_maze1");
    // The return value from system() is always an exit status.
    // However, exits with a signal seem to get folded onto the low eight bits.
    cr_assert(WEXITSTATUS(ret) & 0x80, "The program did not exit with a signal.");
    int exp = SIGUSR2;
    cr_assert_eq(WEXITSTATUS(ret) & 0x7f, exp, "The program exited with signal %d (%d expected)",
		 WEXITSTATUS(ret) & 0x7f, exp);
}

/*
 * Invoke the server with a port argument and an empty maze template.
 * The server may continue anyway (and be killed with SIGUSR2) or exit with EXIT_FAILURE,
 * but not any other signal.
 */
Test(main_suite, empty_template, .timeout = 5, .init = link_exec) {
#ifdef NO_MAIN
    cr_assert_fail("The main module was not implemented");
#endif
    int ret;
    ret = system("(sleep 1; killall -q -USR2 bin/mzw4) & bin/mzw4 -p 9003 -t tests/rsrc/empty_maze");

    // Allow the program to be terminated with SIGUSR2, but not any other signal.
    if(WEXITSTATUS(ret) & 0x80) {
	cr_assert_eq(WEXITSTATUS(ret) & 0x7f, SIGUSR2, "The program exited with an unexpected signal (%d)",
		     WEXITSTATUS(ret) & 0x7f);
    } else {
	// If the program does exit, then it should be with EXIT_FAILURE.
	int exp = 1;
	cr_assert_eq(WEXITSTATUS(ret) & 0x7f, exp, "The program exited with status %d (%d expected)",
		     WEXITSTATUS(ret) & 0x7f, exp);
    }
}

/*
 * Invoke the server with a port argument and an ill-formed maze template.
 * The server may continue anyway (and be killed with SIGUSR2) or exit with EXIT_FAILURE,
 * but not any other signal.
 */
Test(main_suite, bad_template, .timeout = 5, .init = link_exec) {
#ifdef NO_MAIN
    cr_assert_fail("The main module was not implemented");
#endif
    int ret;
    ret = system("(sleep 1; killall -q -USR2 bin/mzw5) & bin/mzw5 -p 9004 -t tests/rsrc/bad_maze1");

    // Allow the program to be terminated with SIGUSR2, but not any other signal.
    if(WEXITSTATUS(ret) & 0x80) {
	cr_assert_eq(WEXITSTATUS(ret) & 0x7f, SIGUSR2, "The program exited with an unexpected signal (%d)",
		     WEXITSTATUS(ret) & 0x7f);
    } else {
	// If the program does exit, then it should be with EXIT_FAILURE.
	int exp = 1;
	cr_assert_eq(WEXITSTATUS(ret) & 0x7f, exp, "The program exited with status %d (%d expected)",
		     WEXITSTATUS(ret) & 0x7f, exp);
    }
}

/*
 * Invoke the server with a port argument and a non-existent maze template.
 * The server may continue anyway (and be killed with SIGUSR2) or exit with EXIT_FAILURE,
 * but not any other signal.
 */
Test(main_suite, nonexistent_template, .timeout = 5, .init = link_exec) {
#ifdef NO_MAIN
    cr_assert_fail("The main module was not implemented");
#endif
    int ret;
    ret = system("(sleep 1; killall -q -USR2 bin/mzw6) & bin/mzw6 -p 9005 -t tests/rsrc/nonexistent_maze");

    // Allow the program to be terminated with SIGUSR2, but not any other signal.
    if(WEXITSTATUS(ret) & 0x80) {
	cr_assert_eq(WEXITSTATUS(ret) & 0x7f, SIGUSR2, "The program exited with an unexpected signal (%d)",
		     WEXITSTATUS(ret) & 0x7f);
    } else {
	// If the program does exit, then it should be with EXIT_FAILURE.
	int exp = 1;
	cr_assert_eq(WEXITSTATUS(ret) & 0x7f, exp, "The program exited with status %d (%d expected)",
		     WEXITSTATUS(ret) & 0x7f, exp);
    }
}

/*
 * Invoke the server with a port argument and a maze with no empty space.
 * Loading should succeed.
 */
Test(main_suite, no_space_template, .timeout = 5, .init = link_exec) {
#ifdef NO_MAIN
    cr_assert_fail("The main module was not implemented");
#endif
    int ret;
    ret = system("(sleep 1; killall -q -USR2 bin/mzw7) & bin/mzw7 -p 9006 -t tests/rsrc/bad_maze2");
    // The return value from system() is always an exit status.
    // However, exits with a signal seem to get folded onto the low eight bits.
    cr_assert(WEXITSTATUS(ret) & 0x80, "The program did not exit with a signal.");
    int exp = SIGUSR2;
    cr_assert_eq(WEXITSTATUS(ret) & 0x7f, exp, "The program exited with signal %d (%d expected)",
		 WEXITSTATUS(ret) & 0x7f, exp);
}
