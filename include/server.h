#ifndef SERVER_H
#define SERVER_H

#include "client_registry.h"

/*
 * If nonzero and compiled for debugging, show the maze after each packet processed.
 */
extern int debug_show_maze;

/*
 * Thread function for the thread that handles a particular client.
 *
 * @param  Pointer to a variable that holds the file descriptor for
 * the client connection.  This pointer must be freed once the file
 * descriptor has been retrieved.
 *
 * This function executes a "service loop" that receives packets from
 * the client and dispatches to appropriate functions to carry out
 * the client's requests.  It also maintains information about whether
 * the client has logged in or not.  Until the client has logged in,
 * only LOGIN packets will be honored.  Once a client has logged in,
 * LOGIN packets will no longer be honored, but other packets will be.
 * The service loop ends when the network connection shuts down and
 * EOF is seen.  This could occur either as a result of the client
 * explicitly closing the connection, a timeout in the network causing
 * the connection to be closed, or the main thread of the server shutting
 * down the connection as part of graceful termination.
 */
void *mzw_client_service(void *arg);

#endif
