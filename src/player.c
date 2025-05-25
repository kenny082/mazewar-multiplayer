#include "player.h"
#include "protocol.h"
#include "maze.h"
#include "csapp.h"
#include "debug.h"
#include <time.h>

static const OBJECT valid_avatars[] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z'
};
static const int dr[NUM_DIRECTIONS] = {-1, 0, 1, 0};
static const int dc[NUM_DIRECTIONS] = {0, -1, 0, 1};

#define NUM_AVATARS 26 // hard limit available avatars to upper-case alphabetic characters (A-Z)

struct player{
    OBJECT avatar; // player's avatar (-a)
    char *name; // player's name (-u)
    int fd; // client socket file descriptor
    int row; // row
    int col; // column
    DIRECTION gaze; // facing direction
    int score; // current score
    char (*prev_view)[VIEW_WIDTH]; // previous view for incremental updates
    int prev_depth; // depth of prev_view
    int refcount; // reference counting
    pthread_mutex_t mutex;
    pthread_t thread_id; // thread ID of specific player
    volatile sig_atomic_t hit_pending; // flag set by SIGUSR1 to mark laser hit
};

static pthread_mutex_t players_mutex; // shared mutex for players
static PLAYER *players[NUM_AVATARS]; // array of player structs containing 26 max
static void signal_no_restart(int signum, handler_t *handler){
    struct sigaction action;
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0; // no SA_RESTART
    if(sigaction(signum, &action, NULL) < 0){
        unix_error("signal_no_restart error");
    }
}
static void player_laser_handler(int sig){
    pthread_t me = pthread_self();
    for (int i = 0; i < NUM_AVATARS; i++){
        PLAYER *player = players[i];
        if(player && pthread_equal(player->thread_id, me)){
            player->hit_pending = 1;
            break;
        }
    }
}

// Initialize player module with SIGUSR1 handler, mutex, and avatar array
void player_init(void) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&players_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    for (int i = 0; i < NUM_AVATARS ; i++){ // initialize avatar map to NULL
        players[i] = NULL;
    }
    signal_no_restart(SIGUSR1, player_laser_handler);
}

// Clean up player by forcefully decrementing ref
void player_fini(void) {
    pthread_mutex_lock(&players_mutex);
    for (int i = 0; i < NUM_AVATARS; i++) {
        if (players[i]) {
            // Decrement ref counter until 0 for each player
            while (players[i]->refcount > 0)
                player_unref(players[i], "player_fini");
        }
    }
    pthread_mutex_unlock(&players_mutex);
    pthread_mutex_destroy(&players_mutex);
}

// Initializes a new player
PLAYER *player_login(int clientfd, OBJECT avatar, char *name) {
    char *real_name;
    if(name && name[0] != '\0'){
        if(name[0] < 'A' || name[0] > 'Z'){
            return NULL;
        }
        real_name = strdup(name);
    }
    else{
        real_name = strdup("Anonymous");
    }
    if(!real_name) return NULL;
    if(avatar != 0){
        if(!((avatar >= 'A' && avatar <= 'Z') || (avatar >= 'a' && avatar <= 'z'))){
            free(real_name);
            return NULL;
        }
    }
    OBJECT requested_avatar = avatar;
    if (requested_avatar >= 'a' && requested_avatar <= 'z'){
        requested_avatar = (OBJECT)toupper((unsigned char)requested_avatar);
    }
    OBJECT real_avatar = 0;
    pthread_mutex_lock(&players_mutex);
    if(IS_AVATAR(requested_avatar) && players[requested_avatar - 'A'] == NULL){
        real_avatar = requested_avatar;
    }
    else{
        OBJECT first = (name && name[0] != '\0') ? (OBJECT)real_name[0] : 0;
        if(IS_AVATAR(first) && players[first - 'A'] == NULL){
            real_avatar = first;
        }
        else{
            for (int i = 0; i < NUM_AVATARS; i++){
                OBJECT potential_avatar = valid_avatars[i];
                if (players[potential_avatar - 'A'] == NULL){
                    real_avatar = potential_avatar;
                    break;
                }
            }
        }
    }
    if (!IS_AVATAR(real_avatar)){
        pthread_mutex_unlock(&players_mutex);
        free(real_name);
        return NULL;
    }
    PLAYER *player = malloc(sizeof(*player)); // allocate PLAYER* then initialize values for player
    if(!player){
        pthread_mutex_unlock(&players_mutex);
        free(real_name);
        return NULL;
    }
    player->avatar = real_avatar;
    player->name = real_name; // duplicate names are allowed
    player->fd = clientfd;
    player->row = -1;
    player->col = -1;
    player->gaze = EAST; // seems to be EAST everytime player is spawned on util/mazewar
    player->score = 0;
    player->prev_view = NULL;
    player->prev_depth = 0;
    player->refcount = 1;
    player->hit_pending = 0;
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&player->mutex, &mattr);
    pthread_mutexattr_destroy(&mattr);
    players[real_avatar - 'A'] = player;
    player->thread_id = pthread_self(); // record self ID for laser hits tracking
    pthread_mutex_unlock(&players_mutex);
    return player;
}

// Logs a player out by removing player and decrementing reference
void player_logout(PLAYER *player) {
    int row, col, dir;
    if (player_get_location(player, &row, &col, &dir) == 0) {
        maze_remove_player(player->avatar, row, col);
        for (int i = 0; i < NUM_AVATARS; i++) {
            PLAYER *p = players[i];
            if (p) player_update_view(p);
        }
    }
    // Send a score packet of -1
    MZW_PACKET pkt;
    pkt.type = MZW_SCORE_PKT;
    pkt.param1 = player->avatar;
    pkt.param2 = -1;
    pkt.param3 = 0;
    pkt.size = 0;
    pthread_mutex_lock(&players_mutex);
    for (int i = 0; i < NUM_AVATARS; i++){
        PLAYER *p = players[i];
        if (p && p != player){
            player_send_packet(p, &pkt, NULL);
        }
    }
    players[player->avatar - 'A'] = NULL;
    pthread_mutex_unlock(&players_mutex);
    player_unref(player, "player_logout");
}

// Resets the player as if they just joined, reset "stats" and randomly place on maze again
void player_reset(PLAYER *player) {
    int row, col, dir;
    if (player_get_location(player, &row, &col, &dir) == 0) { // Remove the player if present
        maze_remove_player(player->avatar, row, col);
    }
    // Attempt to randomly place if an empty spot is found
    if (maze_set_player_random(player->avatar, &row, &col) != 0) {
        // If failure, force the client service thread to ungracefully shutdown to force termination of service
        shutdown(player->fd, SHUT_RD);
        return;
    }
    player->row = row;
    player->col = col;
    // Perform full view update instead of incremental upon player reset
    for (int i = 0; i < NUM_AVATARS; i++) {
        PLAYER *p = players[i];
        if (p) {
            player_invalidate_view(p);
            player_update_view(p);
        }
    }
    // Refresh current player's scoreboard, send this data to all other players connected
    for (int i = 0; i < NUM_AVATARS; i++) {
        PLAYER *p = players[i];
        if (!p) continue;
        size_t name_len = strlen(p->name);
        MZW_PACKET pkt = {
            .type = MZW_SCORE_PKT,
            .param1 = p->avatar,
            .param2 = p->score,
            .param3 = 0,
            .size = (uint16_t)name_len
        };
        player_send_packet(player, &pkt, p->name);
    }
    for (int i = 0; i < NUM_AVATARS; i++) {
        PLAYER *p = players[i];
        if (p == NULL || p == player) continue;
        size_t name_len = strlen(p->name);
        MZW_PACKET pkt = {
            .type = MZW_SCORE_PKT,
            .param1 = player->avatar,
            .param2 = player->score,
            .param3 = 0,
            .size = (uint16_t)name_len
        };
        player_send_packet(p, &pkt, p->name);
    }
}

// Looks up a player by avatar, increments its reference count, returns that PLAYER if it exists else NULL
PLAYER *player_get(unsigned char avatar){
    // Restrict to upper case A-Z characters
    if (!IS_AVATAR(avatar)) {
        return NULL;
    }
    pthread_mutex_lock(&players_mutex);
    PLAYER *player = players[avatar - 'A'];
    if (player) {
        player = player_ref(player, "player_get"); // increment ref for caller
    }
    pthread_mutex_unlock(&players_mutex);
    return player;
}
    
// Increase reference count of player by one
PLAYER *player_ref(PLAYER *player, char *why){
    pthread_mutex_lock(&player->mutex);
    player->refcount++;
    debug("player_ref: %s now %d [%s]\n", player->name, player->refcount, why);
    pthread_mutex_unlock(&player->mutex);
    return player;
}

// Decrement reference count and free if zero
void player_unref(PLAYER *player, char *why) {
    pthread_mutex_lock(&player->mutex);
    player->refcount--;
    debug("player_unref: %s now %d [%s]\n", player->name, player->refcount, why);
    if (player->refcount == 0) {
        pthread_mutex_unlock(&player->mutex);
        pthread_mutex_lock(&players_mutex);
        players[player->avatar - 'A'] = NULL;
        pthread_mutex_unlock(&players_mutex);
        free(player->name);
        if (player->prev_view) {
            free(player->prev_view);
        }
        pthread_mutex_destroy(&player->mutex);
        free(player);
        return;
    }
    pthread_mutex_unlock(&player->mutex);
}

// Sends packet via proto_send_packet
int player_send_packet(PLAYER *player, MZW_PACKET *pkt, void *data) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    pkt->timestamp_sec  = (uint32_t)ts.tv_sec;
    pkt->timestamp_nsec = (uint32_t)ts.tv_nsec;
    int rc;
    pthread_mutex_lock(&player->mutex);
    rc = proto_send_packet(player->fd, pkt, data);
    pthread_mutex_unlock(&player->mutex);
    return rc;
}

// Grabs player position and gaze direction
int player_get_location(PLAYER *player, int *rowp, int *colp, int *dirp){
    int row = player->row;
    int col = player->col;
    int max_rows = maze_get_rows();
    int max_cols = maze_get_cols();
    if(row < 0 || row >= max_rows || col < 0 || col >= max_cols){
        return -1;
    }
    *rowp = row;
    *colp = col;
    *dirp = player->gaze;
    return 0;
}

// Attempt to move player avatar forward/backward one unit of distance (dir = sign)
int player_move(PLAYER *player, int dir){
    int row, col, gaze;
    // Grab position + direction of gaze, fails if not within bound
    if (player_get_location(player, &row, &col, &gaze) != 0) {
        return -1;
    }
    DIRECTION move_direction = (dir == 1 ? gaze : REVERSE(gaze)); // current gaze if dir = 1, else reverse 
    int move = maze_move(row, col, move_direction);
    // If move successful, must update this information to all clients
    if (move == 0) { // calculate delta after move
        player->row = row + dr[move_direction];
        player->col = col + dc[move_direction];
        // Update view incrementally
        for (int i = 0; i < NUM_AVATARS; i++) {
            PLAYER *p = players[i];
            if (p) {
                // player_invalidate_view(p);
                player_update_view(p);
            }
        }
    }
    return move;
}

// Rotate player gaze 90 degrees (CCW  = 1, CW = -1)
void player_rotate(PLAYER *player, int dir) {
    pthread_mutex_lock(&player->mutex);
    if (dir == 1) {
        player->gaze = TURN_LEFT(player->gaze); // left turn or CCW
    } else {
        player->gaze = TURN_RIGHT(player->gaze); // right turn or CW
    }
    player_invalidate_view(player);
    player_update_view(player);
    pthread_mutex_unlock(&player->mutex);
}

void player_fire_laser(PLAYER *player) {
    int row, col, dir;
    // Check if player is within bounds/exists
    if (player_get_location(player, &row, &col, &dir) != 0) {
        return;
    }
    OBJECT target = maze_find_target(row, col, player->gaze);  // returns first avatar fund or EMPTY
    if (IS_AVATAR(target)) {
        // Find target if maze_find_target found the target avatar to shoot the laser at
        PLAYER *victim = player_get(target);
        if (victim) {
            pthread_kill(victim->thread_id, SIGUSR1); // sends SIGUSR1 to victim thread_id
            player_unref(victim, "player_fire_laser");
        }
        // Increment self score
        pthread_mutex_lock(&player->mutex);
        player->score++;
        pthread_mutex_unlock(&player->mutex);
        // Update the scores for all clients after a laser hit
        MZW_PACKET pkt = {
            .type = MZW_SCORE_PKT,
            .param1 = player->avatar,
            .param2 = player->score,
            .param3 = 0,
            .size = 0
        };
        pthread_mutex_lock(&players_mutex);
        for (int i = 0; i < NUM_AVATARS; i++) {
            PLAYER *p = players[i];
            if (p){
                player_send_packet(p, &pkt, NULL);
            }
        }
        pthread_mutex_unlock(&players_mutex);
    }
}

// Invalidate current view to force full update instead of incremental
void player_invalidate_view(PLAYER *player){
    pthread_mutex_lock(&player->mutex);
    if (player->prev_view) { // free previous if exists
        free(player->prev_view);
        player->prev_view = NULL;
    }
    player->prev_depth = 0; // set prev_depth to 0
    pthread_mutex_unlock(&player->mutex);
}

void player_update_view(PLAYER *player){
    int row, col, gaze;
    player_get_location(player, &row, &col, &gaze); // grab current position + direction of gaze
    // allocate space for new vice X3D and generate it
    char (*new_view)[VIEW_WIDTH] = Malloc(VIEW_DEPTH * sizeof new_view[0]);
    pthread_mutex_lock(&player->mutex);
    int new_depth = maze_get_view((VIEW *)new_view, row, col, player->gaze, VIEW_DEPTH );   
    pthread_mutex_unlock(&player->mutex);
    int full_update = (player->prev_view == NULL || player->prev_depth != new_depth); // see if need full or incremental update
    if (full_update) { // if full update, clear board, then resend full view
        MZW_PACKET clear = {MZW_CLEAR_PKT, 0, 0, 0, 0};
        player_send_packet(player, &clear, NULL);
        // display all cells in new view
        for (int d = 0; d < new_depth; d++) {
            for (int w = 0; w < VIEW_WIDTH; w++) {
                // obj = object type, w = (l-wall,corridor,r-wall), d = depth, 0 = size of payload
                MZW_PACKET show = {MZW_SHOW_PKT, new_view[d][w], w, d, 0};
                player_send_packet(player, &show, NULL);
            }
        }
    } else {
        // Show incremental changed cells only, don't clear view
        for (int d = 0; d < new_depth; d++) {
            for (int w = 0; w < VIEW_WIDTH; w++) {
                char nc = new_view[d][w];
                char oc = (d < player->prev_depth ? player->prev_view[d][w] : '\0');
                if (nc == oc) continue;
                MZW_PACKET show = {MZW_SHOW_PKT, nc, w, d, 0};
                player_send_packet(player, &show, NULL);
            }
        }
    }
    pthread_mutex_lock(&player->mutex);
    if (player->prev_view) free(player->prev_view);
    player->prev_view = new_view;
    player->prev_depth = new_depth;
    pthread_mutex_unlock(&player->mutex);
}

// Check if a player got hit via hit_pending flag
void player_check_for_laser_hit(PLAYER *player) {
    int hit = 0;
    pthread_mutex_lock(&player->mutex);
    if (player->hit_pending) { hit = 1; player->hit_pending = 0; }
    pthread_mutex_unlock(&player->mutex);
    if (!hit) return;
    int r, c, d; // if hit, remove from avatar from location, update other players view about this change
    if (player_get_location(player, &r, &c, &d) == 0) {
        maze_remove_player(player->avatar, r, c);
        for (int i = 0; i < NUM_AVATARS; i++) {
            PLAYER *p = players[i];
            if (p) player_update_view(p);
        }
    }
    MZW_PACKET alert = {.type = MZW_ALERT_PKT, .param1 = 0, .param2 = 0, .param3 = 0, .size = 0}; // send alert packet
    player_send_packet(player, &alert, NULL);
    // freezes for 3 seconds, then reset player into maze
    sleep(3);
    player_reset(player);
}

void player_send_chat(PLAYER *player, char *msg, size_t len){
    size_t name_len = strlen(player->name);
    size_t prefix_len = name_len + 4;  // '[' + avatar + ']' + ' ' (basically [avatar] _ where _ is whitespace)
    size_t total_len  = prefix_len + len;
    char *buf = Malloc(total_len);
    memcpy(buf, player->name, name_len); // copy username, and format the [] brackets
    buf[name_len + 0] = '[';
    buf[name_len + 1] = player->avatar;
    buf[name_len + 2] = ']';
    buf[name_len + 3] = ' ';
    memcpy(buf + prefix_len, msg, len); // copy their msg
    MZW_PACKET pkt = {
        .type = MZW_CHAT_PKT,
        .param1 = 0,
        .param2 = 0,
        .param3 = 0,
        .size = (uint16_t)total_len
    };
    pthread_mutex_lock(&players_mutex);
    // Send this structured chat packet to all players
    for (int i = 0; i < NUM_AVATARS; i++) {
        PLAYER *p = players[i];
        if (p) {
            player_send_packet(p, &pkt, buf);
        }
    }
    pthread_mutex_unlock(&players_mutex);
    free(buf);
}