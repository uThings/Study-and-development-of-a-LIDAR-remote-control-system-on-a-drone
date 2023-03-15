#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "../common/common_lib.h"
#include "tcpsocketlib.h"
#include "server_lib.h"

#define PATH_START "../../data_generation/start.sh"
#define PATH_STOP  "../../data_generation/stop.sh"
#define LOGGER_NAME  "voxl-logger"

int server_handler (int csk, char *ip_addr, int port)
{
    buffer_tcp b = init_buffer_tcp();
    char cmd [BUFSIZ+1];

    pid_t pid = 0;
    int active_connection = 1;

    printf("connected to %s: %d\n", ip_addr, port);

    if ((pid = fork()) == -1) {
        printf("Unable to create child process\n");
        exit (EXIT_FAILURE);
    }

    if (pid != 0) {

        /* parent */

        printf("Child process created (pid = %X)\n", pid);
        return 1;   /* keep parent server alive */
    }
    else {

        /* child */

        while (active_connection) {
            /* wait for cmd from the client */
            tcp_readline(csk, &b, cmd);

            if (strcmp(cmd, "STATUS") != 0)
                printf("TCP receive: %s\n", cmd);

            if (!strcmp(cmd, "STATUS")) {
                Status(csk, LOGGER_NAME);
            }
            else if (!strncmp(cmd, "START", 5)) {
                Start(csk, cmd, PATH_START, LOGGER_NAME);
            } 
            else if (!strcmp(cmd, "STOP")) {
                Stop(csk, PATH_STOP, LOGGER_NAME);
            } 
            else if (!strcmp(cmd, "SEND_NEXT")) {
                SendNext(csk, LOGGER_NAME, DATA_DIR);
            }
            else if (!strncmp(cmd, "REMOVE_FILE", 11)) {
                RemoveFile(csk, cmd, DATA_DIR);
            }
            else if (!strncmp(cmd, "REMOVE_DIR", 10)) {
                RemoveDir(csk, cmd, DATA_DIR);
            }
            else if (!strncmp(cmd, "SEND_FILE", 9)) {
                SendFile(csk, cmd);
            }
            else if (!strcmp(cmd, "CLOSE")) {
                active_connection = 0;
            }
            else {
                tcp_send(csk, "Not valid command\n");
                printf("Not valid command (%s)\n", cmd);
            }
        }

        printf("Closing child server process...\n");
        return 0;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "args: server_ip_address, tcp_port\n");
        exit(EXIT_FAILURE);
    }

    /* avoid child processes becoming zombies */
    signal(SIGCHLD, SIG_IGN);

    if (create_tcp_server (argv[1], atoi (argv[2])) < 0) {
        fprintf(stderr, "Error creating the TCP server\n");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}

void error_handler (const char *message)
{
    printf("fatal error: %s\n", message);
    exit(EXIT_FAILURE);
}