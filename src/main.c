/*
 *  This file is part of tils.
 *
 *  tils is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  tils is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with tils.  If not, see <http://www.gnu.org/licenses/>
 */

/**
 * @file src/main.c
 *
 * @brief Instantiate server here
 *
 * @author Lars Wander (lars.wander@gmail.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lib/util.h>
#include <tils/routes.h>
#include <tils/worker_thread.h>
#include <tils/tils.h>


int main(int argc, char *argv[]) {
    int server_fd = 0;
    int res = 0;
    int port = 80;

    if (argc > 1) {
        char *end = NULL;
        long res = strtol(argv[1], &end, 10);
        if (*end != '\0' || res < 0 || res >= (1 << 16) - 1) {
            fprintf(stderr, ERROR "Invalid port %s\n", argv[1]);
            exit(-1);
        }
        port = (int)res;
    }

    fprintf(stdout, "Building routes...\n");
    if (tils_routes_init() < 0 || 
           tils_route_add("/", "html/index.html") < 0 ||
           tils_route_add("/apple-touch-icon.png", "html/apple-touch-icon.png") < 0 ||
           tils_route_add("/favicon.png", "html/favicon.png") < 0 ||
           tils_route_add("/common.css", "html/common.css") < 0 ||
           tils_route_add("/test/test.html", "html/test/test.html") < 0) {
        fprintf(stdout, "failed.\n");
        res = -1;
        goto cleanup_routes;
    } 

    fprintf(stdout, "Opening connection on port %d...\n", port);
    if ((server_fd = init_server(port)) < 0) {
        fprintf(stdout, "failed.\n");
        res = -1;
        goto cleanup_routes;
    }

    fprintf(stdout, "Starting working threads...\n");
    tils_start_thread_pool(server_fd);

    close(server_fd);

cleanup_routes:
    tils_routes_cleanup();

    fprintf(stdout, "Goodbye\n");
    return res;
}
