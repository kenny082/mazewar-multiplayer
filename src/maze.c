#include "maze.h"
#include "csapp.h"

static int maze_rows;
static int maze_cols;
static OBJECT **maze_cells;
static pthread_mutex_t maze_mutex;
static const int dr[NUM_DIRECTIONS] = {-1, 0, 1, 0};
static const int dc[NUM_DIRECTIONS] = {0, -1, 0, 1};

// Initialize maze (row,col), populate the array using template, initialize MUTEX, set SRAND
void maze_init(char **template) {
    int rows;
    for (rows = 0; template[rows] != NULL; rows++); // count rows not NULL (NULL denotes end of MAZE generation)
    maze_rows = rows;
    if(maze_rows == 0){ // maze must be empty
        maze_cols = 0;
        maze_cells = NULL;
        pthread_mutex_init(&maze_mutex, NULL);
        return;
    }
    maze_cols = strlen(template[0]);
    for (rows = 1; rows < maze_rows; rows++){ // ensure rectangular-shape maze
        if((int)strlen(template[rows]) != maze_cols){
            fprintf(stderr, "Maze is not in rectangular shape, inconsistent row lengths in template\n");
            exit(EXIT_FAILURE);
        }
    }
    OBJECT **cells = Malloc(maze_rows * sizeof(char*));
    for(rows = 0; rows < maze_rows; rows++){
        cells[rows] = Malloc((maze_cols + 1) * sizeof(char));
        memcpy(cells[rows], template[rows], maze_cols + 1);
    }
    pthread_mutex_init(&maze_mutex, NULL);
    pthread_mutex_lock(&maze_mutex);
    maze_cells = cells;
    pthread_mutex_unlock(&maze_mutex);
    srand(time(NULL)); // seed the pseudo-random number generator for random player in maze placement
}

// Free maze and destroy maze mutex
void maze_fini() {
    pthread_mutex_lock(&maze_mutex);
    if (maze_cells != NULL){ // maze exist, free whole maze
        for (int rows = 0; rows < maze_rows; rows++){
            Free(maze_cells[rows]);
        }
        Free(maze_cells);
        maze_cells = NULL;
    }
    pthread_mutex_unlock(&maze_mutex);
    pthread_mutex_destroy(&maze_mutex);
}
// Getters for the maze rows and columns (total amount)
int maze_get_rows() {
    return maze_rows;
}

int maze_get_cols() {
    return maze_cols;
}

// Set the player with avatar at (row,col)
int maze_set_player(OBJECT avatar, int row, int col) {
    int result;
    pthread_mutex_lock(&maze_mutex);
    // Check bounds and position must be empty
    if (row < 0 || row >= maze_rows || col < 0 || col >= maze_cols || !IS_EMPTY(maze_cells[row][col])){
        result = 1; // could not find a spot
    }
    else{
        maze_cells[row][col] = avatar;
        result = 0; // was able to find a spot and place avatar
    }
    pthread_mutex_unlock(&maze_mutex);
    return result;
}

// Randomly set player with avatar at (row,col) repeatedly until success or give up
int maze_set_player_random(OBJECT avatar, int *rowp, int *colp) {
    if (maze_rows == 0 || maze_cols == 0){
        return 1;
    }
    pthread_mutex_lock(&maze_mutex);
    int empty_count = 0; // precompute empty spaces and place randomly over these spots
    for (int rows = 0; rows < maze_rows; rows++){
        for (int columns = 0; columns < maze_cols; columns++){
            if(maze_cells[rows][columns] == EMPTY){
                empty_count++;
            }
        }
    }
    if (empty_count == 0){ // no space to place on maze
        pthread_mutex_unlock(&maze_mutex);
        *rowp = *colp = -1;
        return 1;
    }
    typedef struct{
        int row,col;
    } Position;
    // Scan for empty spots, create a struct containg all valid locations, pick a random location from the valid spots
    Position *empties = Malloc(empty_count * sizeof(Position)); // create an arary of struct containg valid (row,col)
    int index = 0;
    for (int rows = 0; rows < maze_rows; rows++){
        for (int columns = 0; columns < maze_cols; columns++){
            if (maze_cells[rows][columns] == EMPTY){
                empties[index++] = (Position){.row = rows, .col = columns};
            }
        }
    }
    int pick = rand() % empty_count;
    int pick_r = empties[pick].row;
    int pick_c = empties[pick].col;
    maze_cells[pick_r][pick_c] = avatar;
    *rowp = pick_r;
    *colp = pick_c;
    Free(empties);
    pthread_mutex_unlock(&maze_mutex);
    return 0;
}

// Set position to empty (' ') if player is present there and within bound
void maze_remove_player(OBJECT avatar, int row, int col) {
    pthread_mutex_lock(&maze_mutex);
    if (row >= 0 && row < maze_rows && col >=0 && col < maze_cols && maze_cells[row][col] == avatar){
        maze_cells[row][col] = EMPTY;
    }
    pthread_mutex_unlock(&maze_mutex);
}

int maze_move(int row, int col, int dir) {
    pthread_mutex_lock(&maze_mutex);    
    if (row < 0 || row >= maze_rows || col < 0 || col >= maze_cols || dir < 0 || dir >= NUM_DIRECTIONS){
        pthread_mutex_unlock(&maze_mutex);
        return 1;
    }
    OBJECT object = maze_cells[row][col]; // check if this object is the avatar
    if(!IS_AVATAR(object)){
        pthread_mutex_unlock(&maze_mutex);
        return 1;
    }
    // At this point we are within bound, legal direction to move, and player exists
    // Calculate destination coordinates and ensure it still within bounds + empty
    int new_row = row + dr[dir];
    int new_col = col + dc[dir];
    if (new_row < 0 || new_row >= maze_rows || new_col < 0 || new_col >= maze_cols || !IS_EMPTY(maze_cells[new_row][new_col])){
        pthread_mutex_unlock(&maze_mutex);
        return 1;
    }
    maze_cells[row][col] = EMPTY; // set to empty as player has moved from the cell
    maze_cells[new_row][new_col] = object; // set player to new coordinates verified within bounds and empty
    pthread_mutex_unlock(&maze_mutex);
    return 0;
}

// Search from (row, col) in dir until AVATAR and returns AVATAR, otherwise EMPTY
OBJECT maze_find_target(int row, int col, DIRECTION dir) {  
    pthread_mutex_lock(&maze_mutex);
    int r = row + dr[dir];
    int c = col + dc[dir];
    while(r >= 0 && r < maze_rows && c >= 0 && c < maze_cols){
        OBJECT object = maze_cells[r][c];
        if (!IS_EMPTY(object)){
            pthread_mutex_unlock(&maze_mutex);
            return IS_AVATAR(object) ? object : EMPTY; // if avatar return AVATAR, else return empty
        }
        r += dr[dir];
        c += dc[dir];
    }
    pthread_mutex_unlock(&maze_mutex);
    return EMPTY;
}

int maze_get_view(VIEW *view, int row, int col, DIRECTION gaze, int depth) {
    int distance;
    pthread_mutex_lock(&maze_mutex);
    for (distance = 0; distance < depth; distance++) {
        // Compute corridor cells 
        int row_distance = row + dr[gaze] * distance;
        int column_distance = col + dc[gaze] * distance;
        if (row_distance < 0 || row_distance >= maze_rows ||
            column_distance < 0 || column_distance >= maze_cols) {
            break;
        }
        DIRECTION left  = TURN_LEFT(gaze);
        DIRECTION right = TURN_RIGHT(gaze);
        // Copy the X3D rectangular view
        (*view)[distance][LEFT_WALL] = maze_cells[row_distance + dr[left]][column_distance + dc[left]];
        (*view)[distance][CORRIDOR] = maze_cells[row_distance][column_distance];
        (*view)[distance][RIGHT_WALL] = maze_cells[row_distance + dr[right]][column_distance + dc[right]];
        // If not empty, include it, and stop
        if (distance > 0 && (*view)[distance][CORRIDOR] != EMPTY) {
            distance++;
            break;
        }
    }
    pthread_mutex_unlock(&maze_mutex);
    return distance;
}

// Debug purposes, prints a X3D view
void show_view(VIEW *view, int depth) {
    // view[distance][LEFT_WALL], view[distance][CORRIDOR], view[distance][RIGHT_WALL]
    for (int distance = 0; distance < depth; distance++){
        fprintf(stderr, "[%2d] %c %c %c\n", distance, (*view)[distance][LEFT_WALL], (*view)[distance][CORRIDOR],(*view)[distance][RIGHT_WALL]);
    }
}

// Debug purposes, display the state of the maze
void show_maze() {
    pthread_mutex_lock(&maze_mutex);
    fprintf(stderr, "rows=%d, cols=%d\n", maze_rows, maze_cols); // total rows and columns in maze
    for (int r = 0; r < maze_rows; r++) {
        fprintf(stderr, "%s\n", maze_cells[r]);
    }
    pthread_mutex_unlock(&maze_mutex);
}