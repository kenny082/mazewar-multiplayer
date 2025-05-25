#include <criterion/criterion.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "debug.h"
#include "protocol.h"
#include "excludes.h"

#define SERVER_PORT 9998
#define SERVER_HOSTNAME "localhost"

/*
 * Initialize a packet.
 *   pkt - pointer to header
 *   type - packet type
 *   size - size of payload
 */
static void proto_init_packet(MZW_PACKET *pkt, MZW_PACKET_TYPE type, size_t size) {
    memset(pkt, 0, sizeof(*pkt));
    pkt->type = type;
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
	perror("clock_gettime");
    }
    pkt->timestamp_sec = ts.tv_sec;
    pkt->timestamp_nsec = ts.tv_nsec;
    pkt->size = size;
}

/*
 * Get the server address.
 * This is done once, so we don't have to worry about the library
 * functions being thread safe.
 */
static struct in_addr server_addr;

static int get_server_address(void) {
    struct hostent *he;
    if((he = gethostbyname(SERVER_HOSTNAME)) == NULL) {
	perror("gethostbyname");
	return -1;
    }
    memcpy(&server_addr, he->h_addr, sizeof(server_addr));
    return 0;
}

/*
 * Connect to the server.
 *
 * Returns: connection file descriptor in case of success.
 * Returns -1 and sets errno in case of error.
 */
static int proto_connect(void) {
    struct sockaddr_in sa;
    int sfd;

    if((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	return(-1);
    }
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(SERVER_PORT);
    memcpy(&sa.sin_addr.s_addr, &server_addr, sizeof(server_addr));
    if(connect(sfd, (struct sockaddr *)(&sa), sizeof(sa)) < 0) {
	perror("connect");
	close(sfd);
	return(-1);
    }
    return sfd;
}

/*
 * This should be a test of the server at the client interface level.
 * The basic idea is to feed the client command lines and analyze the responses.
 *
 * WARNING: These tests are coordinated to all run concurrently.
 * You must use --jobs XXX where XXX is sufficiently large to allow them all to
 * run, otherwise there will be issues.  The sleep times in the tests also have
 * to be adjusted if any changes are made.
 */

static void init() {
#ifndef NO_SERVER
    int ret;
    int i = 0;
    do { // Wait for server to start
	ret = system("netstat -an | fgrep '0.0.0.0:9998' > /dev/null");
	sleep(1);
    } while(++i < 30 && WEXITSTATUS(ret));
#endif
}

static void fini() {
}

/*
 * Thread to run a command using system() and collect the exit status.
 */
static void *system_thread(void *arg) {
    long ret = system((char *)arg);
    return (void *)ret;
}

// Criterion seems to sort tests by name.  This one can't be delayed
// or others will time out.
Test(server_suite, 00_start_server, .timeout = 60) {
#ifdef NO_SERVER
    cr_assert_fail("Server was not implemented");
#endif
    fprintf(stderr, "server_suite/00_start_server\n");
    int server_pid = 0;
    int ret = system("netstat -an | fgrep '0.0.0.0:9998' > /dev/null");
    cr_assert_neq(WEXITSTATUS(ret), 0, "Server was already running");
    fprintf(stderr, "Starting server...");
    if((server_pid = fork()) == 0) {
	execlp("valgrind", "mazewar", "--leak-check=full", "--track-fds=yes",
	       "--error-exitcode=37", "--log-file=test_output/valgrind.out", "bin/mazewar", "-p", "9998", NULL);
	fprintf(stderr, "Failed to exec server\n");
	abort();
    }
    fprintf(stderr, "pid = %d\n", server_pid);
    char *cmd = "sleep 40";
    pthread_t tid;
    pthread_create(&tid, NULL, system_thread, cmd);
    pthread_join(tid, NULL);
    cr_assert_neq(server_pid, 0, "Server was not started by this test");
    fprintf(stderr, "Sending SIGHUP to server pid %d\n", server_pid);
    kill(server_pid, SIGHUP);
    sleep(5);
    kill(server_pid, SIGKILL);
    wait(&ret);
    fprintf(stderr, "Server wait() returned = 0x%x\n", ret);
    if(WIFSIGNALED(ret)) {
	fprintf(stderr, "Server terminated with signal %d\n", WTERMSIG(ret));	
	system("cat test_output/valgrind.out");
	if(WTERMSIG(ret) == 9)
	    cr_assert_fail("Server did not terminate after SIGHUP");
    }
    if(WEXITSTATUS(ret) == 37)
	system("cat test_output/valgrind.out");
    cr_assert_neq(WEXITSTATUS(ret), 37, "Valgrind reported errors");
    cr_assert_eq(WEXITSTATUS(ret), 0, "Server exit status was not 0");
}

Test(server_suite, 01_connect_disconnect, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_SERVER
    cr_assert_fail("Server was not implemented");
#endif
    int ret = system("util/tclient -p 9998 </dev/null | grep 'Connected to server'");
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
}

Test(server_suite, 02_connect_login, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_SERVER
    cr_assert_fail("Server was not implemented");
#endif
    // The tclient program terminates due to EOF on the input immediately after the expected
    // READY packet is received.
    int ret = system("(echo 'login A Alice' | util/tclient -q -p 9998) 2>&1 | grep 'type=READY' > /dev/null");
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
}

Test(server_suite, 03_connect_login_refresh, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_SERVER
    cr_assert_fail("Server was not implemented");
#endif
    // The tclient program terminates due to EOF on the input immediately after the expected
    // READY packet is received.
    int ret = system("(cat tests/rsrc/03_login_refresh | util/tclient -q -p 9998 > test_output/03_login_refresh.out) 2>&1");
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
}

Test(server_suite, 03_connect_login_turn, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_SERVER
    cr_assert_fail("Server was not implemented");
#endif
    // The tclient program terminates due to EOF on the input immediately after the expected
    // READY packet is received.
    int ret = system("(cat tests/rsrc/03_login_turn | util/tclient -q -p 9998 > test_output/03_login_turn.out) 2>&1");
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
}

/*
 * End-to-end stress test.
 * Start several clients, which will concurrently issue random commands and keep track
 * of SCORE packets that are sent.  After this goes on for awhile, the clients will
 * disconnect and return their final scoreboards, which will be checked for agreement.
 */

#define NUM_CLIENTS (10)
#define NUM_ITERS (1000)

static char *user_names[NUM_CLIENTS] = {
    "Alice", "Bob", "Cindy", "Dave", "Ellen", "Fred", "Ginger", "Harry", "Isabelle", "John"
};

struct client_thread_args {
    int fd;
    char avatar;
    int iters;
    int8_t scoreboard[NUM_CLIENTS];
    int8_t final_scoreboard[NUM_CLIENTS];
};

/*
 * An audit thread receives packets from the server and processes them
 * to track aspects of the game state.  For now, we are just tracking the
 * scoreboard.
 */

static void *audit_thread(void *arg) {
    // An audit thread shares the argument structure with a client thread,
    // but they operate on disjoint parts of it.
    //pthread_detach(pthread_self());
    struct client_thread_args *ap = arg;
    debug("audit '%c' starting (fd = %d)", ap->avatar, ap->fd);
    MZW_PACKET pkt;
    void *payload;
    int old, new;
    while(!proto_recv_packet(ap->fd, &pkt, &payload)) {
	switch(pkt.type) {
	case MZW_SCORE_PKT:
	    old = ap->scoreboard[pkt.param1-'A'];
	    new = pkt.param2;
	    // In the reference solution, I do not observe a player's score
	    // ever to be decreasing.  If we do see this when testing code,
	    // it probably indicates that something is wrong.
	    cr_assert(new >= old || new == -1,
		      "Player '%c' score has decreased from %d to %d\n",
		      ap->avatar, old, new);
	    cr_assert(new <= old+1,
		      "Player '%c' score has jumped from %d to %d\n",
		      ap->avatar, old, new);
	    // If a player has logged out, save the final score for later comparison.
	    if(new == -1)
	      ap->final_scoreboard[pkt.param1-'A'] = old;
	    ap->scoreboard[pkt.param1-'A'] = new;
	    break;
	default:
	    break;
	}
	if(payload)
	    free(payload);
    }
    // An audit thread reaches here when there is a send error.
    // This means the connection has been shut down.
    debug("audit '%c' terminating (final score: %d)",
	   ap->avatar, ap->final_scoreboard[ap->avatar-'A']);

    // Show the final scoreboard, for diagnostic purposes.
    // The scoreboards of clients that terminate later should be monotonically
    // greater than those of clients that terminate earlier.
#if DEBUG
    for(int i = 0; i < NUM_CLIENTS; i++)
      printf("%c:%d ", 'A'+i, ap->scoreboard[i]);
    printf("\n");
#endif
    return NULL;
}

/*
 * A client thread connects to the server and logs in using a particular avatar.
 * It then sends packets to execute a random series of commands.
 */

static void *client_thread(void *arg) {
    struct client_thread_args *ap = arg;
    int ret;
    MZW_PACKET pkt;
    // Connect to server
    debug("client '%c' starting", ap->avatar);
    ap->fd = proto_connect();
    cr_assert(ap->fd > 0, "Client '%c' connect to server failed", ap->avatar);
    debug("client '%c' connected (fd = %d)", ap->avatar, ap->fd);

    // Send login
    char *uname = user_names[(int)ap->avatar-'A'];
    proto_init_packet(&pkt, MZW_LOGIN_PKT, strlen(uname));
    pkt.param1 = ap->avatar;
    ret = proto_send_packet(ap->fd, &pkt, uname);
    cr_assert_eq(ret, 0);

    // We have to wait for the READY, otherwise there is a race.
    void *payload;
    if(proto_recv_packet(ap->fd, &pkt, &payload) || pkt.type != MZW_READY_PKT) {
	cr_assert_fail("%c: READY was not received in response to LOGIN", ap->avatar);
    }

    // Start audit thread
    pthread_t tid;
    pthread_create(&tid, NULL, audit_thread, ap);

    // Now run main loop
    unsigned int seed = 1;
    while(--ap->iters) {
	char msg[128];
	switch(rand_r(&seed) % 6) {
	case 0:  // forward
	    proto_init_packet(&pkt, MZW_MOVE_PKT, 0);
	    pkt.param1 = 1;
	    ret = proto_send_packet(ap->fd, &pkt, NULL);
	    cr_assert_eq(ret, 0);
	    break;
	case 1:  // back
	    proto_init_packet(&pkt, MZW_MOVE_PKT, 0);
	    pkt.param1 = -1;
	    ret = proto_send_packet(ap->fd, &pkt, NULL);
	    cr_assert_eq(ret, 0);
	    break;
	case 2:  // left
	    proto_init_packet(&pkt, MZW_TURN_PKT, 0);
	    pkt.param1 = 1;
	    ret = proto_send_packet(ap->fd, &pkt, NULL);
	    cr_assert_eq(ret, 0);
	    break;
	case 3:  // right
	    proto_init_packet(&pkt, MZW_TURN_PKT, 0);
	    pkt.param1 = -1;
	    ret = proto_send_packet(ap->fd, &pkt, NULL);
	    cr_assert_eq(ret, 0);
	    break;
	case 4:  // fire
	    proto_init_packet(&pkt, MZW_FIRE_PKT, 0);
	    ret = proto_send_packet(ap->fd, &pkt, NULL);
	    cr_assert_eq(ret, 0);
	    break;
	case 5:  // send
	    snprintf(msg, sizeof(msg), "'%c' iteration %d", ap->avatar, ap->iters);
	    proto_init_packet(&pkt, MZW_SEND_PKT, strlen(msg));
	    ret = proto_send_packet(ap->fd, &pkt, NULL);
	    cr_assert_eq(ret, 0);
	    break;
	default:
	    break;
	}
    }
    cr_assert(ap->iters == 0, "Client '%c' terminating prematurely", ap->avatar);
    // Shutdown connection and wait for audit thread to terminate
    debug("client '%c' waiting", ap->avatar);
    // Shutdown the write side and wait for the server to terminate the connection.
    // Otherwise there can be backlog at the server that gets pounded out on a
    // broken connection, causing lots of distracting error messages and we don't
    // collect the packets.
    shutdown(ap->fd, SHUT_WR);
    pthread_join(tid, NULL);
    debug("client '%c' terminating", ap->avatar);
    close(ap->fd);
    return NULL;
}

Test(server_suite, 05_stress_test, .init = init, .fini = fini, .timeout = 30) {
#ifdef NO_SERVER
    cr_assert_fail("Server was not implemented");
#endif
    // Delay for a bit so we don't overlap the other tests.
    sleep(5);
    pthread_t tid[NUM_CLIENTS];
    struct client_thread_args *args[NUM_CLIENTS];
    for(int i = 0; i < NUM_CLIENTS; i++)
	args[i] = calloc(1, sizeof(struct client_thread_args));

    // Set up the server address.
    if(get_server_address())
	abort();

    // Spawn client threads and then wait for them to finish.
    for(int i = 0; i < NUM_CLIENTS; i++) {
	struct client_thread_args *ap = args[i];
	ap->avatar = 'A' + i;
	ap->iters = NUM_ITERS;
	pthread_create(&tid[i], NULL, client_thread, ap);
    }
    for(int i = 0; i < NUM_CLIENTS; i++)
	pthread_join(tid[i], NULL);

    // Compare final scoreboards.
    // A client that logged out earlier might not have seen all the score updates
    // that a client that logged out later has seen.  However, if j logged out
    // later than i, then the final score that j has for i should agree with the
    // final score that i has for itself.  We can determine whether j logged out
    // later than i by the situation in which args[j]->scoreboard[i] == -1 but
    // args[i]->scoreboard[j] >= 0.
    for(int i = 0; i < NUM_CLIENTS; i++) {
      for(int j = 0; j < NUM_CLIENTS; j++) {
	if(args[j]->scoreboard[i] == -1 && args[i]->scoreboard[j] >= 0)
	  cr_assert_eq(args[j]->final_scoreboard[i], args[i]->final_scoreboard[i],
		       "scoreboard [%c][%c] (== %d) does not match scoreboard [%c][%c] (== %d)",
		       j+'A', i+'A', args[j]->final_scoreboard[i], i+'A', i+'A',
		       args[i]->final_scoreboard[i]);
      }
    }
}
