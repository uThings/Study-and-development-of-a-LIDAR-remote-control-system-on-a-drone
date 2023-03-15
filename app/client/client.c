#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "../common/common_lib.h"
#include "tcpsocketlib.h"
#include "client_lib.h"
#include "io_lib.h"

#define MS_DELAY 100
#define LOG_FILE "lidar_control.log"
#define LOG_FILE_MODE "w"

int server_handler (int csk, char *ip_addr, int port) {}

int main (int argc, char *argv[])
{
    int control_sk;
    char cmd[MAXCMDLEN];
    char rec[BUFSIZ+1];
    char timestamp[TIMESTAMP_LEN];
    char ch;

    int active_connection = 0;
    int end_client = 0;
    int logger_status = 0;
    int transmission_status;
    thread_data t;
    pthread_t thread_id;
    struct tr_info tr;
    buffer_tcp control_buffer = init_buffer_tcp();

    /* ignore SIGPIPE in order to enter the error_handler after write() tries to write on a broken socket */
    signal(SIGPIPE, SIG_IGN);

    if ((t.log_file = fopen(LOG_FILE, LOG_FILE_MODE)) == NULL) {
        fprintf(stderr, "Impossible to open the log file %s\n", LOG_FILE);
        exit(EXIT_FAILURE);
    }

    /* set the log file to line buffered mode */
    char log_buf[1024];
    setvbuf(t.log_file, log_buf, _IOLBF, sizeof(log_buf));

    t.running = t.status = 0;
    strcpy(t.ip, argv[1]);
    t.port = atoi(argv[2]);

    if (pthread_mutex_init(&(t.lock), NULL) != 0) {
        fprintf(stderr, "Mutex init failed\n");
        exit(EXIT_FAILURE);
    }

    if (argc != 3) {
        fprintf(stderr, "args: server_ip_address tcp_port\n");
        exit(EXIT_FAILURE);
    }

    /* init the I/O */
    InitIO();
    PrintLog(t.log_file, &(t.lock), "Starting the system\n");

    do {

        /* control connection */
        control_sk = Connect(argv[1], atoi(argv[2]), &active_connection, &end_client, &t);

        while (active_connection && !end_client) {
            /* get status */
            if (TcpSendReceiveMT(control_sk, &control_buffer, "STATUS\n", rec, &t, thread_id) < 0) {
                active_connection = 0;
                break;
            }
            sscanf(rec, "%*s %d", &logger_status);

            /* check if the transmission thread has to be resumed */
            if (t.status == -2) {
                if (CreateTransmissionThread(control_sk, &t, &thread_id) < 0) {
                    ExitMainThread(control_sk, &t, thread_id, EXIT_FAILURE);    
                }
            }

            if ((ch = GetUserInput()) != EOF) {

                if (ch == '1') {
                    PrintLog(t.log_file, &(t.lock), "Sending START command\n");
                    TimestampString(timestamp, "%Y%m%d-%H%M%S");
                    snprintf(cmd, MAXCMDLEN, "START %s\n", timestamp);
                    if (TcpSendReceiveMT(control_sk, &control_buffer, cmd, rec, &t, thread_id) < 0) {
                        active_connection = 0;
                        break;
                    }
                }

                if (ch == '2') {
                    PrintLog (t.log_file, &(t.lock), "Sending STOP command\n");
                    if (TcpSendReceiveMT(control_sk, &control_buffer, "STOP\n", rec, &t, thread_id) < 0) {
                        active_connection = 0;
                        break;
                    }
                }

                if (ch == '3') {
                    if (t.status == 0) {
                        if (CreateTransmissionThread(control_sk, &t, &thread_id) < 0) {
                            ExitMainThread(control_sk, &t, thread_id, EXIT_FAILURE);    
                        }
                    }
                }

                if (ch == '4') {
                    if (t.status == 1) {
                        t.running = 0;
                        PrintLog(t.log_file, &(t.lock), "Stopping the transmission thread\n");
                    }
                }

                if (ch == '5') {
                    end_client = 1;
                }
            }

            if (t.status == -1) {
                PrintLog(t.log_file, &(t.lock), "ERROR detected on transmission thread, ending main thread\n");
                ExitMainThread(control_sk, &t, thread_id, EXIT_FAILURE); 
            }

            /* copy data safely */
            pthread_mutex_lock(&(t.lock));
            tr = t.tr;
            transmission_status = t.status;
            pthread_mutex_unlock(&(t.lock));

            PrintInfo(logger_status, transmission_status, tr);

            sleepms(MS_DELAY);
        }
           
    } while (!end_client);

    ExitMainThread(control_sk, &t, thread_id, EXIT_SUCCESS);

    return EXIT_SUCCESS;
}

void error_handler (const char *message)
{
    /* the error handler does nothing */
}
