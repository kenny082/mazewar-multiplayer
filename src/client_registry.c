#include "client_registry.h"
#include "csapp.h"

#define MAX_CLIENTS FD_SETSIZE
CLIENT_REGISTRY *client_registry;

struct client_registry {
    int client_fds[MAX_CLIENTS]; // array of client sockfd, -1 = empty
    int client_count; // count of total clients connected
    pthread_mutex_t mutex;
    sem_t empty; // semaphore to block until client_count == 0 (creg_wait_for_empty)
};

CLIENT_REGISTRY *creg_init() {
    CLIENT_REGISTRY *cr = Malloc(sizeof(CLIENT_REGISTRY));
    for (int i = 0; i < MAX_CLIENTS; i++) { // zero out array upon initilization
        cr->client_fds[i] = -1;
    }
    cr->client_count = 0;
    Sem_init(&cr->empty, 0, 0);
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&cr->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    return cr;
}

void creg_fini(CLIENT_REGISTRY *cr) {
    pthread_mutex_destroy(&cr->mutex);
    sem_destroy(&cr->empty);
    Free(cr);
}

void creg_register(CLIENT_REGISTRY *cr, int fd) {
    pthread_mutex_lock(&cr->mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (cr->client_fds[i] == -1) {
            cr->client_fds[i] = fd;
            cr->client_count++;
            break;
        }
    }
    pthread_mutex_unlock(&cr->mutex);
}

void creg_unregister(CLIENT_REGISTRY *cr, int fd) {
    pthread_mutex_lock(&cr->mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (cr->client_fds[i] == fd) {
            cr->client_fds[i] = -1;
            cr->client_count--;
            break;
        }
    }
    // if client_count == 0, call V to wake up blocked threads in creg_wait_for_empty
    if (cr->client_count == 0) {
        V(&cr->empty);
    }
    pthread_mutex_unlock(&cr->mutex);
}

void creg_wait_for_empty(CLIENT_REGISTRY *cr) {
    P(&cr->empty);  // wait until the registry becomes empty
}

void creg_shutdown_all(CLIENT_REGISTRY *cr) {
    pthread_mutex_lock(&cr->mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (cr->client_fds[i] >= 0) { // check if sockfd occupied, if so use shutdown() with SHUT_RD
            shutdown(cr->client_fds[i], SHUT_RD);  // cause EOF in client handler and terminate
        }
    }
    pthread_mutex_unlock(&cr->mutex);
}