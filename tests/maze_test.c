#include <criterion/criterion.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "debug.h"
#include "maze.h"
#include "excludes.h"

/* Number of threads we create in multithreaded tests. */
#define NTHREAD (10)

/* Number of iterations we use in several tests. */
#define NITER (1000000)

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

static void init_default() {
    maze_init(default_maze);
}

// A 5x10 empty maze.
static char *empty_maze[] = {
  "          ",
  "          ",
  "          ",
  "          ",
  "          ",
  NULL
};

static void init_empty() {
    maze_init(empty_maze);
}

Test(maze_suite, get_rows_test, .init = init_default, .timeout = 5) {
#ifdef NO_MAZE
    cr_assert_fail("Maze module was not implemented");
#endif
    int rows = maze_get_rows();
    int exp = (sizeof(default_maze)/sizeof(char *)) - 1;
    cr_assert_eq(rows, exp, "Expected %d, was %d", exp, rows);
}

Test(maze_suite, get_cols_test, .init = init_default, .timeout = 5) {
#ifdef NO_MAZE
    cr_assert_fail("Maze module was not implemented");
#endif
    int cols = maze_get_cols();
    int exp = strlen(default_maze[0]);
    cr_assert_eq(cols, exp, "Expected %d, was %d", exp, cols);
}

// Set player at 0, 0
Test(maze_suite, set_player_test_1, .init = init_empty, .timeout = 5) {
#ifdef NO_MAZE
    cr_assert_fail("Maze module was not implemented");
#endif
    int ret = maze_set_player('A', 0, 0);
    int exp = 0;
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
}

// Set player at 4, 9
Test(maze_suite, set_player_test_2, .init = init_empty, .timeout = 5) {
#ifdef NO_MAZE
    cr_assert_fail("Maze module was not implemented");
#endif
    int ret = maze_set_player('A', 4, 9);
    int exp = 0;
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
}

// Set two players at the same location
Test(maze_suite, set_player_test_3, .init = init_empty, .timeout = 5) {
#ifdef NO_MAZE
    cr_assert_fail("Maze module was not implemented");
#endif
    int ret = maze_set_player('A', 3, 5);
    int exp = 0;
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    ret = maze_set_player('B', 3, 5);
    cr_assert_neq(ret, 0, "Value returned was 0");
}

#ifdef NOTDEF
// Probably not fair because specs don't say what should happen.
// Set player at 4, 10 (out of bounds)
Test(maze_suite, set_player_test_4, .init = init_empty, .timeout = 5) {
#ifdef NO_MAZE
    cr_assert_fail("Maze module was not implemented");
#endif
    int ret = maze_set_player('A', 4, 10);
    cr_assert_neq(ret, 0, "Value returned was 0");
}
#endif

// Test that an avatar at 0, 0 can move in the expected directions.
Test(maze_suite, move_test_1, .init = init_empty, .timeout = 5) {
#ifdef NO_MAZE
    cr_assert_fail("Maze module was not implemented");
#endif
    int ret = maze_set_player('A', 0, 0);
    int exp = 0;
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    ret = maze_move(0, 0, NORTH);
    cr_assert_neq(ret, 0, "Value returned was 0");
    ret = maze_move(0, 0, WEST);
    cr_assert_neq(ret, 0, "Value returned was 0");
    ret = maze_move(0, 0, SOUTH);
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    ret = maze_move(1, 0, EAST);
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
}

// Test that an avatar at 4, 9 can move in the expected directions.
Test(maze_suite, move_test_2, .init = init_empty, .timeout = 5) {
#ifdef NO_MAZE
    cr_assert_fail("Maze module was not implemented");
#endif
    int ret = maze_set_player('A', 4, 9);
    int exp = 0;
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    ret = maze_move(4, 9, EAST);
    cr_assert_neq(ret, 0, "Value returned was 0");
    ret = maze_move(4, 9, SOUTH);
    cr_assert_neq(ret, 0, "Value returned was 0");
    ret = maze_move(4, 9, NORTH);
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    ret = maze_move(3, 9, WEST);
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
}

// Test that an avatar at 0, 0 blocked by another avatar, can move
// in the expected directions.
Test(maze_suite, move_test_3, .init = init_empty, .timeout = 5) {
#ifdef NO_MAZE
    cr_assert_fail("Maze module was not implemented");
#endif
    int ret = maze_set_player('A', 0, 0);
    int exp = 0;
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    ret = maze_set_player('B', 1, 0);
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    ret = maze_move(0, 0, SOUTH);
    cr_assert_neq(ret, 0, "Value returned was 0");
    ret = maze_move(1, 0, EAST);
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
}

// Test that removing an avatar removes obstruction to another.
Test(maze_suite, remove_test, .init = init_empty, .timeout = 5) {
#ifdef NO_MAZE
    cr_assert_fail("Maze module was not implemented");
#endif
    int ret = maze_set_player('A', 0, 0);
    int exp = 0;
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    ret = maze_set_player('B', 1, 0);
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    ret = maze_move(1, 0, NORTH);
    cr_assert_neq(ret, 0, "Value returned was 0");
    maze_remove_player('A', 0, 0);
    ret = maze_move(1, 0, NORTH);
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
}

Test(maze_suite, find_target_test_1, .init = init_empty, .timeout = 5) {
#ifdef NO_MAZE
    cr_assert_fail("Maze module was not implemented");
#endif
    int ret = maze_set_player('A', 0, 0);
    int exp = 0;
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    ret = maze_set_player('B', 0, 9);
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    exp = EMPTY;
    ret = maze_find_target(0, 0, SOUTH); // Eventually out of bounds
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    exp = 'B';
    ret = maze_find_target(0, 0, EAST); // Should hit 'B'
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    exp = 0;
    ret = maze_set_player('C', 0, 8);
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    exp = 'C';
    ret = maze_find_target(0, 0, EAST); // Should hit 'C'
}

// Immediate out-of-bounds in finding target.
Test(maze_suite, find_target_test_2, .init = init_empty, .timeout = 5) {
#ifdef NO_MAZE
    cr_assert_fail("Maze module was not implemented");
#endif
    int ret = maze_set_player('A', 0, 0);
    int exp = 0;
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    exp = EMPTY;
    ret = maze_find_target(0, 0, NORTH); // Immediately out of bounds
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    ret = maze_find_target(0, 0, WEST); // Immediately out of bounds
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
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

static VIEW empty_view = {
    {EMPTY, 'A', EMPTY}, 
    {EMPTY, EMPTY, EMPTY}, 
    {EMPTY, EMPTY, EMPTY}, 
    {EMPTY, EMPTY, EMPTY}, 
    {EMPTY, EMPTY, EMPTY}, 
    {EMPTY, EMPTY, EMPTY}, 
    {EMPTY, EMPTY, EMPTY}, 
    {EMPTY, EMPTY, EMPTY}, 
    {EMPTY, EMPTY, EMPTY}, 
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY}
};

Test(maze_suite, view_test_1a, .init = init_empty, .timeout = 5) {
#ifdef NO_MAZE
    cr_assert_fail("Maze module was not implemented");
#endif
    int ret = maze_set_player('A', 0, 0);
    int exp = 0;
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    char view[VIEW_DEPTH][VIEW_WIDTH];
    // Specs do not say what to do when side of view goes beyond maze bounds.
    memset(view, EMPTY, sizeof(view));
    // Looking NORTH from 0,0 should produce a view of depth 1.
    exp = 1;
    ret = maze_get_view(&view, 0, 0, NORTH, VIEW_DEPTH);
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    cr_assert_eq(compare_view(&view, &empty_view, ret), 0, "View did not match expected");
}

Test(maze_suite, view_test_1b, .init = init_empty, .timeout = 5) {
#ifdef NO_MAZE
    cr_assert_fail("Maze module was not implemented");
#endif
    int ret = maze_set_player('A', 0, 0);
    int exp = 0;
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    char view[VIEW_DEPTH][VIEW_WIDTH];
    // Specs do not say what to do when side of view goes beyond maze bounds.
    memset(view, EMPTY, sizeof(view));
    // Looking SOUTH from 0,0 should produce a view of depth 5.
    exp = 5;
    memset(view, EMPTY, sizeof(view));
    ret = maze_get_view(&view, 0, 0, SOUTH, VIEW_DEPTH);
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    cr_assert_eq(compare_view(&view, &empty_view, ret), 0, "View did not match expected");
}

Test(maze_suite, view_test_1c, .init = init_empty, .timeout = 5) {
#ifdef NO_MAZE
    cr_assert_fail("Maze module was not implemented");
#endif
    int ret = maze_set_player('A', 0, 0);
    int exp = 0;
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    char view[VIEW_DEPTH][VIEW_WIDTH];
    // Specs do not say what to do when side of view goes beyond maze bounds.
    memset(view, EMPTY, sizeof(view));
    // Looking EAST from 0,0 should produce a view of depth 10.
    exp = 10;
    memset(view, EMPTY, sizeof(view));
    ret = maze_get_view(&view, 0, 0, EAST, VIEW_DEPTH);
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    cr_assert_eq(compare_view(&view, &empty_view, ret), 0, "View did not match expected");
}

static VIEW east_view = {
    {EMPTY, 'A', EMPTY}, 
    {'$',   EMPTY, '@'}, 
    {'$',   EMPTY, '@'}, 
    {'$',   EMPTY, '@'}, 
    {'$',   EMPTY, '@'}, 
    {'$',   EMPTY, '@'}, 
    {'$',   EMPTY, '@'}, 
    {EMPTY, EMPTY, '@'}, 
    {'$',   EMPTY, '@'}, 
    {'$',   EMPTY, '@'},
    {'$',   EMPTY, '@'},
    {'$',   EMPTY, '@'},
    {'$',   EMPTY, '@'},
    {'$',   EMPTY, '@'},
    {'$',   EMPTY, '@'},
    {'$',   EMPTY, '@'}
};

static VIEW north_view = {
    {'#',   'A', EMPTY}, 
    {EMPTY, EMPTY, '$'}, 
    {'%',   '%',   '%'}, 
    {'%',   '%',   '%'}, 
    {'*',   '*',   '*'}, 
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY}
};

Test(maze_suite, view_test_2, .init = init_default, .timeout = 5) {
#ifdef NO_MAZE
    cr_assert_fail("Maze module was not implemented");
#endif
    int ret = maze_set_player('A', 4, 11);
    int exp = 0;
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    char view[VIEW_DEPTH][VIEW_WIDTH];
    // Specs do not say what to do when side of view goes beyond maze bounds.
    memset(view, EMPTY, sizeof(view));
    // Looking EAST from 4,11 should produce a view of maximum depth.
    exp = VIEW_DEPTH;
    ret = maze_get_view(&view, 4, 11, EAST, VIEW_DEPTH);
    cr_assert_eq(ret, exp, "Expected %d, was %d", exp, ret);
    cr_assert_eq(compare_view(&view, &east_view, ret), 0, "View did not match expected");
    // Looking NORTH from 4,11 should produce a view of depth d where 3 <= d <= 5.
    exp = 3;
    ret = maze_get_view(&view, 4, 11, NORTH, VIEW_DEPTH);
    cr_assert(ret >= exp && ret <= 5, "Expected %d <= depth <= 5, was %d", exp, ret);
    cr_assert_eq(compare_view(&view, &north_view, exp), 0, "View did not match expected");
}

/*
 * Concurrency stress test.
 * Several threads, each with their own avatar, move their avatars around
 * the maze, using a separate data structure to keep track of where they
 * ought to be, based on the return values from maze_move().
 * After a large number of moves, all but one of the avatars is removed
 * from the maze, and then an attempt is made to determine the actual position
 * of the remaining one by counting how many steps NORTH and WEST can be taken
 * before movement fails.  The result is compared with the calculated position.
 */

/*
 * Thread that randomly moves an avatar through the maze.
 * The thread delays at the start of the test, to make it more likely
 * that other threads started at about the same time are active.
 */
struct random_motion_args {
    int start_delay; // Starting delay
    char avatar;     // Avatar for this thread
    int row;         // Current row position
    int col;         // Current column position
    int iters;       // Iterations remaining
    int remove;      // Whether avatar should be removed after last iteration
};

/*
 * Maps giving the row/column increments needed to move in each direction.
 */
static int row_incs[] = { -1, 0, 1, 0 };
static int col_incs[] = { 0, -1, 0, 1 };

#define IN_BOUNDS(row, col) ((row) >= 0 && (row) < maze_get_rows() && \
                             (col) >= 0 && (col) < maze_get_cols())

static void random_motion(struct random_motion_args *ap) {
    unsigned int seed = 1;
    while(ap->iters--) {
	int dir = rand_r(&seed) % NUM_DIRECTIONS;
	if(!IN_BOUNDS(ap->row + row_incs[dir], ap->col + col_incs[dir]))
	    continue;
	int ret = maze_move(ap->row, ap->col, dir);
	if(!ret) {  // There could have been a collision
	    ap->row += row_incs[dir];
	    ap->col += col_incs[dir];
	}
    }
}

static void *random_motion_thread(void *arg) {
    struct random_motion_args *ap = arg;
    if(ap->start_delay)
	sleep(ap->start_delay);
    maze_set_player(ap->avatar, ap->row, ap->col);
    random_motion(ap);
    if(ap->remove) {
	maze_remove_player(ap->avatar, ap->row, ap->col);
    }
    return NULL;
}


Test(data_suite, random_motion_test, .init = init_empty, .timeout = 10) {
#ifdef NO_MAZE
    cr_assert_fail("Maze module was not implemented");
#endif
    // Spawn threads to run random ref/unref.
    // To keep the initial positioning simple, we will use only four threads,
    // each initially at one corner of the maze.
    pthread_t tid[4];
    struct random_motion_args *ap0;
    for(int i = 0; i < 4; i++) {
	struct random_motion_args *ap = calloc(1, sizeof(struct random_motion_args));
	if(i == 0)
	    ap0 = ap;  // Save thread 0's argument structure for the end
	ap->start_delay = 1;
	ap->avatar = 'A' + i;
	ap->row = (i < 2 ? 0 : 4);
	ap->col = (i % 2 == 0 ? 0 : 9);
	ap->iters = NITER;
	ap->remove = (i != 0);  // Only the first thread remains in the maze
	pthread_create(&tid[i], NULL, random_motion_thread, ap);
    }

    // Wait for all threads to finish.
    for(int i = 0; i < 4; i++)
	pthread_join(tid[i], NULL);

    // Attempt to determine the position of the avatar for thread 0 and
    // compare with what it is supposed to be.
    while(ap0->row > 0 && !maze_move(ap0->row--, ap0->col, NORTH))
	;
    while(ap0->col > 0 && !maze_move(ap0->row, ap0->col--, WEST))
	;
    cr_assert(ap0->row == 0 && ap0->col == 0,
	      "Actual final position of avatar did not match calculated position");
}
