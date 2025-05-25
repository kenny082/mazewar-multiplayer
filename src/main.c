#include <csapp.h>
#include "client_registry.h"
#include "server.h"
#include "maze.h"
#include "player.h"
#include "debug.h"

static void terminate(int status);
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

static void sighup_handler(int sig) {
  terminate(EXIT_SUCCESS); // call terminate
}

static void signal_no_restart(int signum, handler_t *handler){
  struct sigaction action;
  action.sa_handler = handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0; // no SA_RESTART
  if(sigaction(signum, &action, NULL) < 0){
    unix_error("signal_no_restart error");
  }
}

int main(int argc, char* argv[]){
  char *port = NULL;
  char *template_file = NULL;
  char **maze_template = default_maze; // fall back to hard-coded / default maze if maze_template empty/invalid
  int opt;
  while((opt = getopt(argc, argv, "p:t:")) != -1){
    switch(opt){
      case 'p':{
        char *end;
        long v = strtoul(optarg, &end, 10); 
        // Verify port number is between 1024-65535 (non-restricted valid TCP port numbers)
        if(end == optarg || *end != '\0' || v < 1024UL || v > 65535UL){  
          fprintf(stderr, "ERROR: Port number \"%s\" (must be 1024-65535)\n", optarg);
          exit(EXIT_FAILURE);
        }
        port = optarg;
        break;
      }
      case 't':
        template_file = optarg;
        break;
      default:
        fprintf(stderr, "Usage: util/mazewar [-p <port>] [-t <template file>]");
        exit(EXIT_FAILURE);
    }
  }
  if(!port){
    fprintf(stderr, "ERROR: Missing required -p port option\n");
    exit(EXIT_FAILURE);
  }
  // If optional flag provided, attempt to use template_file
  if (template_file) {
    FILE *f = Fopen(template_file, "r");
    char **lines = NULL;
    int n = 0;
    char buf[MAXLINE];
    while (Fgets(buf, MAXLINE, f)) {
        int len = strlen(buf);
        if (len && buf[len-1] == '\n') buf[--len] = '\0';
        for (int j = 0; j < len; j++){
          if(isupper((unsigned char)buf[j])){
            fprintf(stderr, "ERROR: Invalid character '%c' in maze template (upper case A-Z not allowed, reserved for players)\n"
                          , buf[j]);
            exit(EXIT_FAILURE);
          }
        }
        lines = Realloc(lines, (n+1) * sizeof *lines);
        lines[n++] = strdup(buf);
    }
    Fclose(f);
    if (n > 0) {
        lines = Realloc(lines, (n+1) * sizeof *lines);
        lines[n] = NULL;
        maze_template = lines;
    } else if (lines) {
        Free(lines);
    }
  }
  signal_no_restart(SIGHUP,sighup_handler);
  signal_no_restart(SIGPIPE, SIG_IGN); // prevent CRTL + C on client terminal from killing server
  // Perform required initializations of the client_registry, maze, and player modules
  client_registry = creg_init();
  maze_init(maze_template); // changed from default_maze in the event we may need fallback if no valid -t
  player_init();
  debug_show_maze = 1; // Show the maze after each packet.
  // Server setup with accept loop
  int listenfd = Open_listenfd(port);
  while(1){
    struct sockaddr_storage clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    int connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
    int *connfdp = Malloc(sizeof *connfdp);
    *connfdp = connfd;
    pthread_t tid;
    Pthread_create(&tid, NULL, mzw_client_service, connfdp);
  }
  terminate(EXIT_SUCCESS);
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    // Shutdown all client connections. This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);
    debug("Waiting for service threads to terminate...");
    creg_wait_for_empty(client_registry);
    debug("All service threads terminated.");
    // Finalize modules.
    creg_fini(client_registry);
    player_fini();
    maze_fini();
    debug("MazeWar server terminating");
    exit(status);
}