#include <criterion/criterion.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "debug.h"
#include "protocol.h"
#include "player.h"
#include "maze.h"
#include "excludes.h"

/* Number of threads we create in multithreaded tests. */
#define NTHREAD (10)

/* Number of iterations we use in several tests. */
#define NITER (10000)

// Default maze
static char *default_maze[] = {
  "******************************",
  "***** %%%%%%%%% &&&&&&&&&&& **",
  "***** %%%%%%%%%        $$$$  *",
  "*           $$$$$$ $$$$$$$$$ *",
  "*##########                  *",
  "*########## @@@@@@@@@@@@@@@@@*",
  "*           @@@@@@@@@@@@@@@@@*",
  "******************************",
  NULL
};

// A 1x10 empty maze.
static char *thin_maze[] = {
  "          ",
  NULL
};

// A 5x10 empty maze.
static char *empty_maze[] = {
  "          ",
  "          ",
  "          ",
  "          ",
  "          ",
  NULL
};

static int row_incs[] = { -1, 0, 1, 0 };
static int col_incs[] = { 0, -1, 0, 1 };

#define IN_BOUNDS(row, col) ((row) >= 0 && (row) < maze_get_rows() && \
                             (col) >= 0 && (col) < maze_get_cols())

static int nullfd;
static int filefd;

static void init_null() {
    if((nullfd = open("/dev/null", O_WRONLY, 0777)) < 0) {
	printf("Open failed\n");
	abort();
    }
}

#define PACKET_FILE "test_output/packet.out"

static void init_file() {
    // All tests write concurrently to the same file in append mode.
    // This is probably not very useful.
    if((filefd = open(PACKET_FILE, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777)) < 0) {
	printf("Open failed\n");
	abort();
    }
}

Test(player_suite, player_login_logout, .init = init_null, .timeout = 5) {
#ifdef NO_PLAYER
    cr_assert_fail("Player module was not implemented");
#endif
    maze_init(empty_maze);
    player_init();
    PLAYER *pp = player_login(nullfd, 'A', "Alice");
    cr_assert_not_null(pp, "Expected non-NULL pointer");
    player_logout(pp);
}

// Not fair to test invalid avatar or duplicate avatar because specs did not
// make a clear requirement and the demo code does not necessarily fail in
// these cases.

Test(player_suite, player_reset, .init = init_null, .timeout = 5) {
#ifdef NO_PLAYER
    cr_assert_fail("Player module was not implemented");
#endif
    maze_init(empty_maze);
    player_init();
    PLAYER *pp = player_login(nullfd, 'B', "Bob");
    cr_assert_not_null(pp, "Expected non-NULL pointer");
    int row = -1, col = -1, dir = -1;
    int ret;
    // Before reset, player should have no valid maze location.
    ret = player_get_location(pp, &row, &col, &dir);
    cr_assert_neq(ret, 0, "Return value was zero");
    player_reset(pp);
    ret = player_get_location(pp, &row, &col, &dir);
    cr_assert_eq(ret, 0, "Return value was not zero");
    // Check row, col, and dir to see if they are reasonable.
    cr_assert(IN_BOUNDS(row, col), "Location (%d, %d) is out-of-bounds", row, col);
    cr_assert(dir >= 0 && dir < NUM_DIRECTIONS, "Direction %d is invalid", dir);
}

Test(player_suite, player_move, .init = init_null, .timeout = 5) {
#ifdef NO_PLAYER
    cr_assert_fail("Player module was not implemented");
#endif
    maze_init(empty_maze);
    player_init();
    PLAYER *pp = player_login(nullfd, 'C', "Carol");
    cr_assert_not_null(pp, "Expected non-NULL pointer");
    int row = -1, col = -1, dir = -1;
    int ret;
    player_reset(pp);
    ret = player_get_location(pp, &row, &col, &dir);
    cr_assert_eq(ret, 0, "Return value was not zero");
    // Choose a direction (forward/backward) in which we should be able to move,
    // depending on our direction of gaze.
    int fwd = 1;
    if(!IN_BOUNDS(row + row_incs[dir], col + col_incs[dir]))
	fwd = -1;
    ret = player_move(pp, fwd);
    cr_assert_eq(ret, 0, "Return value was not zero");
    // Now get the new location and see if it makes sense.
    int nrow = -1, ncol = -1, ndir = -1;
    ret = player_get_location(pp, &nrow, &ncol, &ndir);
    cr_assert_eq(ret, 0, "Return value was not zero");
    cr_assert_eq(ndir, dir, "Direction of gaze should not have changed");
    cr_assert_eq(nrow, row + row_incs[dir], "New row number is incorrect");
    cr_assert_eq(ncol, col + col_incs[dir], "New column number is incorrect");
    // Now go back to the original location, which should be in-bounds and vacant.
    fwd = -fwd;
    ret = player_move(pp, fwd);
    cr_assert_eq(ret, 0, "Return value was not zero");
    ret = player_get_location(pp, &nrow, &ncol, &ndir);
    cr_assert_eq(ret, 0, "Return value was not zero");
    cr_assert_eq(ndir, dir, "Direction of gaze should not have changed");
    cr_assert_eq(nrow, row, "Not back at original row");
    cr_assert_eq(ncol, col, "Not back at original column");
}

Test(player_suite, player_rotate, .init = init_null, .timeout = 5) {
#ifdef NO_PLAYER
    cr_assert_fail("Player module was not implemented");
#endif
    maze_init(empty_maze);
    player_init();
    PLAYER *pp = player_login(nullfd, 'D', "Dave");
    cr_assert_not_null(pp, "Expected non-NULL pointer");
    int row = -1, col = -1, dir = -1;
    int ret;
    player_reset(pp);
    ret = player_get_location(pp, &row, &col, &dir);
    cr_assert_eq(ret, 0, "Return value was not zero");
    // Rotate left a full turn, checking the result at each step.
    int exp = dir;
    for(int i = 0; i < NUM_DIRECTIONS; i++) {
	int nrow, ncol, ndir;
	exp = TURN_LEFT(exp);
	player_rotate(pp, 1);
	ret = player_get_location(pp, &nrow, &ncol, &ndir);
	cr_assert_eq(ndir, exp, "New direction is incorrect");
	cr_assert_eq(nrow, row, "Row should not have changed");
	cr_assert_eq(ncol, col, "Column should not have changed");
    }
    // Now rotate right a full turn, checking the result at each step.
    for(int i = 0; i < NUM_DIRECTIONS; i++) {
	int nrow, ncol, ndir;
	exp = TURN_RIGHT(exp);
	player_rotate(pp, -1);
	ret = player_get_location(pp, &nrow, &ncol, &ndir);
	cr_assert_eq(ndir, exp, "New direction is incorrect");
	cr_assert_eq(nrow, row, "Row should not have changed");
	cr_assert_eq(ncol, col, "Column should not have changed");
    }
}

/*
 * Turn player to face a specified direction.
 */
void turn_player_to_face(PLAYER *pp, DIRECTION d) {
    int ret;
    int row, col, dir;
    ret = player_get_location(pp, &row, &col, &dir);
    cr_assert_eq(ret, 0, "Return value was not zero");
    while(dir != d) {
	player_rotate(pp, -1);
	ret = player_get_location(pp, &row, &col, &dir);
	cr_assert_eq(ret, 0, "Return value was not zero");
    }
}

/*
 * Move player to the boundary in the current direction of gaze.
 */
void move_player_to_boundary(PLAYER *pp) {
    int ret;
    int row, col, dir;
    ret = player_get_location(pp, &row, &col, &dir);
    cr_assert_eq(ret, 0, "Return value was not zero");
    while(IN_BOUNDS(row + row_incs[dir], col + col_incs[dir])) {
	player_move(pp, 1);
	ret = player_get_location(pp, &row, &col, &dir);
	cr_assert_eq(ret, 0, "Return value was not zero");
    }
}

Test(player_suite, fire_laser_hit, .init = init_null, .signal = SIGUSR1, .timeout = 5) {
#ifdef NO_PLAYER
    cr_assert_fail("Player module was not implemented");
#endif
    maze_init(thin_maze);
    player_init();
    PLAYER *alice = player_login(nullfd, 'E', "Ellen");
    cr_assert_not_null(alice, "Expected non-NULL pointer");
    player_reset(alice);
    turn_player_to_face(alice, WEST);
    move_player_to_boundary(alice);
    PLAYER *bob = player_login(nullfd, 'F', "Frank");
    cr_assert_not_null(bob, "Expected non-NULL pointer");
    player_reset(bob);
    turn_player_to_face(bob, WEST);
    // Bob should now be gazing at Alice.
    // Firing his laser should now result in a hit.
    // Reset SIGUSR1 handling to default before doing this.
    struct sigaction sact;
    sact.sa_handler = SIG_DFL;
    sigemptyset(&sact.sa_mask);
    sact.sa_flags = 0;
    sigaction(SIGUSR1, &sact, NULL);
    player_fire_laser(bob);
}

Test(player_suite, fire_laser_miss, .init = init_null, .timeout = 5) {
#ifdef NO_PLAYER
    cr_assert_fail("Player module was not implemented");
#endif
    maze_init(thin_maze);
    player_init();
    PLAYER *alice = player_login(nullfd, 'G', "Ginger");
    cr_assert_not_null(alice, "Expected non-NULL pointer");
    player_reset(alice);
    turn_player_to_face(alice, WEST);
    move_player_to_boundary(alice);
    PLAYER *bob = player_login(nullfd, 'H', "Harry");
    cr_assert_not_null(bob, "Expected non-NULL pointer");
    player_reset(bob);
    turn_player_to_face(bob, EAST);
    // Bob should now be gazing away from Alice.
    // Firing his laser should now result in a miss.
    // Reset SIGUSR1 handling to default before doing this.
    struct sigaction sact;
    sact.sa_handler = SIG_DFL;
    sigemptyset(&sact.sa_mask);
    sact.sa_flags = 0;
    sigaction(SIGUSR1, &sact, NULL);
    player_fire_laser(bob);
}

/*
 * Read packets saved to a file and process CLEAR and SHOW packets
 * to calculate the current view.
 */
static void calculate_view(char *name, VIEW *view) {
    int fd;
    fd = open(name, O_RDONLY);
    cr_assert(fd >= 0, "Open file failed");
    MZW_PACKET pkt;
    void *payload;
    while(!proto_recv_packet(fd, &pkt, &payload)) {
	switch(pkt.type) {
	case MZW_CLEAR_PKT:
	    memset(view, EMPTY, VIEW_DEPTH * VIEW_WIDTH);
	    break;
	case MZW_SHOW_PKT:
	    (*view)[pkt.param3][pkt.param2] = pkt.param1;
	    break;
	default:
	    break;
	}
	if(payload)
	    free(payload);
    }
    close(fd);
}

/*
 * Compare two views to a specified depth, returning 0 if equal, nonzero otherwise.
 */
static int compare_view(VIEW *view1, VIEW *view2, int depth) {
    for(int d = 0; d < depth; d++) {
	for(int j = 0; j < VIEW_WIDTH; j++) {
	    char c1 = (*view1)[d][j];
	    char c2 = (*view2)[d][j];
	    if(c1 != c2) {
		printf("Mismatch ('%c' (%d) != '%c' (%d)) at [%d][%d]\n",
		       c1, c1, c2, c2, d, j);
		return 1;
	    }
	}
    }
    return 0;
}

Test(player_suite, move_check_view, .init = init_file, .timeout = 5) {
#ifdef NO_PLAYER
    cr_assert_fail("Player module was not implemented");
#endif
    maze_init(default_maze);
    player_init();
    PLAYER *pp = player_login(filefd, 'I', "Ida");
    cr_assert_not_null(pp, "Expected non-NULL pointer");
    player_reset(pp);
    char displayed_view[VIEW_DEPTH][VIEW_WIDTH];
    char actual_view[VIEW_DEPTH][VIEW_WIDTH];
    unsigned int seed = 1;
    // Move around randomly for awhile and check that the inferred display
    // view matches the actual view extracted from the maze.
    int row, col, dir, depth;
    for(int i = 0; i < 100; i++) {
	if(rand_r(&seed) % 3) { // Rotate
	    int lr = (rand_r(&seed) % 2 ? -1 : 1);
	    player_rotate(pp, lr);
	} else {  // Move
	    int fwd = (rand_r(&seed) % 2 ? -1 : 1);
	    player_move(pp, fwd);
	}
	player_get_location(pp, &row, &col, &dir);
	depth = maze_get_view(&actual_view, row, col, dir, VIEW_DEPTH);
	calculate_view(PACKET_FILE, &displayed_view);
	cr_assert_eq(compare_view(&displayed_view, &actual_view, depth), 0,
		     "Inferred display view does not match actual view");
    }
}

/*
 * Concurrency stress test:
 * Threads that repeatedly runs login/reset/logout, then terminates.
 * Each thread delays at the start of the test, to make it more likely
 * that other threads started at about the same time are active.
 */
struct login_logout_stress_args {
    char avatar;
    int iters;
    int start_delay;
};

static void login_logout_stress(struct login_logout_stress_args *ap) {
    while(ap->iters--) {
	PLAYER *pp = player_login(nullfd, ap->avatar, "Anonymous");
	if(pp == NULL)
	    cr_assert_fail("Login failed for '%c' at iteration %d",
			   ap->avatar, ap->iters);
	player_reset(pp);
	player_logout(pp);
    }
}

static void *login_logout_stress_thread(void *arg) {
    struct login_logout_stress_args *ap = arg;
    if(ap->start_delay)
	sleep(ap->start_delay);
    login_logout_stress(ap);
    return NULL;
}

/*
 * This test is intended to exercise synchronization on the players map,
 * individual player objects, and the maze.  It involves concurrent threads
 * repeatedly logging in, resetting themselves into the maze, then logging out.
 */

Test(player_suite, login_logout_stress, .init = init_null, .timeout = 20) {
#ifdef NO_PLAYER
    cr_assert_fail("Player module was not implemented");
#endif
    maze_init(empty_maze);
    player_init();
    // Spawn threads to run login/logout.
    pthread_t tid[NTHREAD];
    for(int i = 0; i < NTHREAD; i++) {
	struct login_logout_stress_args *ap = calloc(1, sizeof(struct login_logout_stress_args));
	ap->avatar = 'A' + i;
	ap->start_delay = 1;
	ap->iters = NITER;
	pthread_create(&tid[i], NULL, login_logout_stress_thread, ap);
    }

    // Wait for all threads to finish.
    for(int i = 0; i < NTHREAD; i++)
	pthread_join(tid[i], NULL);

    // The test is deemed successful if it completes without crashing, deadlocking,
    // or having any of the logins fail along the way.
}
