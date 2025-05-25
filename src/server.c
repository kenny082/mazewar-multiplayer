#include "server.h"
#include "csapp.h"
#include "protocol.h"
#include "client_registry.h"
#include "maze.h"
#include "player.h"
#include "debug.h"

int debug_show_maze = 0;

static void signal_no_restart(int signum, handler_t *handler){
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if(sigaction(signum, &sa, NULL) <0)
        unix_error("Signal error");
}

void *mzw_client_service(void *arg) {
    int connfd = *((int *)arg);
    Free(arg); // free descriptor storage
    Pthread_detach(Pthread_self()); // detach itself for implict reaping
    creg_register(client_registry, connfd); // register clientfd into creg
    PLAYER *player = NULL;
    while (1) {
        if(player){ // Check if a client is hit first before blocking
            player_check_for_laser_hit(player);
        }
        // Start receiving packets again normally
        MZW_PACKET pkt;
        void *data = NULL;
        int rc = proto_recv_packet(connfd, &pkt, &data);
        if (rc < 0) { // means interrupted by SIGUSR1, loop back for player_check_for_laser_hit to handle it
            if (errno == EINTR){
                continue;
            }
            break;
        }
        // Since not player, must be LOGIN phase, silently ignore other packets until LOGIN packet successful
        if (!player) {
            if (pkt.type == MZW_LOGIN_PKT) {
                OBJECT avatar = pkt.param1; // avatar is parameter 1
                char *name = data ? (char *)data : NULL; // data = name if exist otherwise Anonymous
                PLAYER *p = player_login(connfd, avatar, name);
                MZW_PACKET rsp = {.size = 0};
                if (p) {
                    player = p; // if player logins, send READY packet to the client
                    rsp.type = MZW_READY_PKT;
                    proto_send_packet(connfd, &rsp, NULL);
                    player_reset(player); // place player randomly location in maze
                } else { // send INUSE if unsuccessful LOGIN
                    rsp.type = MZW_INUSE_PKT;
                    proto_send_packet(connfd, &rsp, NULL);
                }
            }
            if (data) Free(data);
            continue;
        }
        // Handle different packet types (MOVE, TURN, FIRE, REFRESH, SEND) POST-LOGIN Successful Phase
        switch (pkt.type) {
            case MZW_MOVE_PKT:
                player_move(player, pkt.param1); // 1 = mv fwd, -1 = mv bkwd
                break;
            case MZW_TURN_PKT:
                player_rotate(player, pkt.param1); // 1 = ccw 90, -1 = cw 90 in degrees
                break;
            case MZW_FIRE_PKT:
                player_fire_laser(player); // fire laser in current direction of gaze
                break;
            case MZW_REFRESH_PKT: // invalidate view to provide full update rather than incremental via update_view
                player_invalidate_view(player);
                player_update_view(player);
                break;
            case MZW_SEND_PKT:
                player_send_chat(player, data ? (char *)data : NULL, pkt.size); // send message to chat for all clients
                break;
            default:
                break; // silently ignore other packet types
        }
        if (data) Free(data);
        if (debug_show_maze) show_maze();
    }
    // clean up after a player disconnects from server
    if (player) {
        player_logout(player);
    }
    creg_unregister(client_registry, connfd);
    Close(connfd);
    return NULL;
}