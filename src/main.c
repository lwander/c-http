/*
 *  This file is part of c-http.
 *
 *  c-http is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  c-http is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with c-http.  If not, see <http://www.gnu.org/licenses/>
 */

/**
 * @file src/main.c
 *
 * @brief Instantiate server here
 *
 * @author Lars Wander (lars.wander@gmail.com)
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <errno.h>

#include <routes.h>
#include <util.h>
#include <socket_util.h>
#include <worker_thread.h>

void sigint_handler(int client_fd) {
    fprintf(stdout, "Shutting down `gracefully'\n");
    close(client_fd);
    exit(-1);
}

/**
 * @brief Bind server to HTTP TCP socket.
 *
 * @return -1 on error, the server file descriptor otherwise.
 */
int init_server() {
    struct sockaddr_in ip4server;
    int server_fd = 0;

    ip4server.sin_family = AF_INET; /* Address family internet */
    ip4server.sin_port = htons(HTTP_PORT); /* Bind to port 80 */
    ip4server.sin_addr.s_addr = htonl(INADDR_ANY);  /* Bind to any interface */

    /* Get a file descriptor for our socket */
    if ((server_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fprintf(stderr, "Unable to create socket (errno %d)\n", errno);
        goto fail;
    }

    if (socket_keepalive(server_fd) < 0) {
        goto fail;
    }

    if (socket_nonblocking(server_fd) < 0) {
        goto fail;
    }

    /* Bind the socket file descriptor to our network interface */
    if (bind(server_fd, (struct sockaddr *)&ip4server, sizeof(ip4server)) < 0) {
        fprintf(stderr, "Unable to bind socket (errno %d)\n", errno);
        goto cleanup_socket;
    }

    /* Listen for connections on this socket. AFAIK, the second argument
     * (backlog) is a suggestion, not a hard value. */
    if (listen(server_fd, 16) < 0) {
        fprintf(stderr, "Unable to listen on socket (errno %d)\n", errno);
        goto cleanup_socket;
    }

    return server_fd;


cleanup_socket:
    close(server_fd);

fail:
    return -1;
}

int main(int argc, char *argv[]) {
    int server_fd = 0;
    int res = 0;

    struct sigaction sigact;
    sigact.sa_handler = sigint_handler;
    sigaction(SIGINT, &sigact, NULL);

    fprintf(stdout, "Building routes... ");
    if (init_routes() < 0 || 
           add_route("/", "html/index.html") < 0 ||
           add_route("/common.css", "html/common.css") < 0) {
        fprintf(stdout, "failed.\n");
        res = -1;
        goto cleanup_routes;
    } else {
        fprintf(stdout, "done.\n");
    }

    fprintf(stdout, "Opening connection... ");
    if ((server_fd = init_server()) < 0) {
        fprintf(stdout, "failed.\n");
        res = -1;
        goto cleanup_routes;
    } else {
        fprintf(stdout, "done.\n");
    }

    fprintf(stdout, "Starting working threads... \n");
    start_thread_pool(server_fd);
    fprintf(stdout, "done.\n");

    while (1) {
        /* TODO emulate console here for grabbing server statitics, or 
         * dedicate this thread to also listen for connections */
    }

    close(server_fd);

cleanup_routes:
    cleanup_routes();

    fprintf(stdout, "Goodbye (errno %d)\n", errno);
    return res;
}
