#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <dirent.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <ncurses.h>
#include "tcpsocketlib.h"
#include "client_lib.h"
#include "../common/compression_lib.h"
#include "io_lib.h"


int tcp_binary_getchar_bs (int sk, buffer_tcp* b, thread_data *t)
{
   if (b->i >= b->dim)
   {
      /* reload the buffer */
      if ((b->dim = tcp_binary_receive (sk, b->buffer)) == -1) {
         /* connection error */
         PrintLog(t->log_file, &(t->lock), "ERROR - TT - Connection error while using tcp_binary_getchar_bs\n");
         ExitTransmissionThread(sk, t, -2);
      }
      b->i = 0;
   }

   return b->buffer[b->i++];
}

void *transmission_thread (void* arg)
{
    int transmission_sk;
    buffer_tcp b = init_buffer_tcp();
    char rec[BUFSIZ+1];
    char file_name[MAX_PATH_LEN];
    char transmission_dir[MAX_REL_PATH_LEN];
    char cmd[MAXCMDLEN];
    char folder[TIMESTAMP_LEN];
    int logger_n;
    int ret;
    struct tr_info tr;
    thread_data *t = (thread_data *)arg;
    t->status = 1;

    long int rec_len;
    long int orig_len;

    /* transmission connection */
    if ((transmission_sk = create_tcp_client_connection(t->ip, t->port)) < 0) {
        PrintLog(t->log_file, &(t->lock), "ERROR - TT - cannot open TCP connection\n");
        ExitTransmissionThread(transmission_sk, t, -1);
    }

    do {
        PrintLog(t->log_file, &(t->lock), "TT - Requesting the next file\n");
        TcpSendReceiveTT(transmission_sk, &b, "SEND_NEXT\n", rec, t);

        while (strncmp(rec, "WAIT", 4) == 0 && t->running) {
            PrintLog(t->log_file, &(t->lock), "TT - Wait received, waiting...\n");
            sleepms(WAIT_DELAY_MS);        
            PrintLog(t->log_file, &(t->lock), "TT - Requesting the next file\n");       
            TcpSendReceiveTT(transmission_sk, &b, "SEND_NEXT\n", rec, t);
        }

        if (strncmp(rec, "SENDING", 7) == 0) {
            
            sscanf(rec, "%*s %s %d %d %d %d %ld %ld", tr.d.first, &tr.d.logger_n, &tr.f.first, &tr.f.last, &tr.f.count, &orig_len, &rec_len);

            /* safely copy the transmission values */
            pthread_mutex_lock(&(t->lock));
            t->tr = tr;
            pthread_mutex_unlock(&(t->lock));
            

            snprintf(transmission_dir, MAX_REL_PATH_LEN, "%s/%s-active", DATA_DIR, tr.d.first);
            if (DirExists(transmission_dir) == 0) {
                CreateTransmissionDir(transmission_dir, tr.d.logger_n);
            }

            snprintf(file_name, MAX_PATH_LEN, "%s/voxl-logger/log%04d/run/mpa/tof/data_%d.pcd", transmission_dir, tr.d.logger_n, tr.f.first);


            PrintLog(t->log_file, &(t->lock), "TT - Receiving %s, compressed size: %ld, original size: %ld\n", file_name, rec_len, orig_len);

            if ((ret = ReceiveAndDecompress(transmission_sk, t, &b, file_name, rec_len, &orig_len)) < 0) {
                PrintLog(t->log_file, &(t->lock), " ERROR - TT - ReceiveAndDecompress() ret: %d\n", ret); 
                ExitTransmissionThread(transmission_sk, t, -1);
            }

            PrintLog(t->log_file, &(t->lock), "TT - Finished writing %s\n", file_name);

            if (ret == 0) {
                PrintLog(t->log_file, &(t->lock), "TT - Request to remove file %d, dir %s\n", tr.f.first, tr.d.first);

                sprintf(cmd, "REMOVE_FILE %s %d %d\n", tr.d.first, tr.d.logger_n, tr.f.first);

                TcpSendReceiveTT(transmission_sk, &b, cmd, rec, t);
            }
        } 

        /* if the transmission is finished, send the other files */
        if (!strncmp(rec, "EMPTY", 5)) {
            sscanf(rec, "%*s %s %d", folder, &logger_n);
            PrintLog(t->log_file, &(t->lock), "TT - Finished sending files (EMPTY)\n");

            /* change to request */
            if (RequestAllReaminingFiles(transmission_sk, &b, t, transmission_dir) < 0) {
                PrintLog(t->log_file, &(t->lock), "ERROR - TT - Failed to requenst remaining files\n");
                ExitTransmissionThread(transmission_sk, t, -1);
            }

            /* remove the dir from the server */
            PrintLog(t->log_file, &(t->lock), "TT - Request to remove %s from the server\n", folder);
            snprintf(file_name, MAX_REL_PATH_LEN, "REMOVE_DIR %s\n", folder);
            TcpSendReceiveTT(transmission_sk, &b, file_name, rec, t);

            /* rename local dir */
            PrintLog(t->log_file, &(t->lock), "TT - Renaming local dir to %s\n", folder);
            snprintf(file_name, MAX_REL_PATH_LEN, "%s/%s", DATA_DIR, folder);
            rename(transmission_dir, file_name);
        }

    } while ((strncmp(rec, "NO_TR", 4) != 0) && t->running);

    PrintLog(t->log_file, &(t->lock), "TT - Transmission finished, ending transmission thread\n");

    ExitTransmissionThread(transmission_sk, t, 0);
}

int ReceiveAndDecompress (int sk, thread_data *t, buffer_tcp* b, const char file_path[], long int rec_len, long int *orig_len)
{
    int i;
    int ret;
    char ch;
    char rec[BUFSIZ+1];
    Bytef* rec_buffer = NULL;

    /* receive the compressed buffer */
    rec_buffer = malloc(rec_len);
    for (i = 0; i < rec_len; i++) {
        rec_buffer[i] = tcp_binary_getchar_bs(sk, b, t);
    }

    if ((ret = tcp_readline(sk, b, rec)) == EOF || ret == 0) {
        /* connection error */
        PrintLog(t->log_file, &(t->lock), "ERROR - TT - Connection error while receiving %s\n", file_path);
        ExitTransmissionThread(sk, t, -2);
    }

    if (strncmp(rec, "END", 3) != 0)
        return -4;

    if ((ret = decompressor(file_path, rec_buffer, rec_len, orig_len)) < 0) {
        free(rec_buffer);
        return ret;
    }

    free(rec_buffer);
    return 0;
}

int RequestFile (int sk, buffer_tcp* b, const char rel_path[], const char data_dir[], thread_data *t) 
{
    int ret;
    char cmd[MAXCMDLEN];
    char buf[BUFSIZ+1];
    char file_path[MAX_PATH_LEN];
    char temp[MAX_PATH_LEN/2];

    long int rec_len;
    long int orig_len;

    sprintf(cmd, "SEND_FILE %s\n", rel_path);
    TcpSendReceiveTT(sk, b, cmd, buf, t);

    if (!strncmp(buf, "SENDING REQUESTED", 17)) {

        sscanf(buf, "%*s %*s %*s %ld %ld", &orig_len, &rec_len);
        sscanf(rel_path, "%*[^/]%s", temp);
        sprintf(file_path, "%s/%s", data_dir, temp);

        return ReceiveAndDecompress(sk, t, b, file_path, rec_len, &orig_len);
    }
    else {
        return -5;
    }

    return 0;
}

int PrintLog (FILE *f, pthread_mutex_t *m, const char format[], ...)
{
    va_list args;
    va_start(args, format);
    int ret;

    char timestamp[26];
    char buffer[MAX_LOG_FORMAT];

    TimestampString(timestamp, "%Y-%m-%d %H:%M:%S");
    snprintf(buffer, MAX_LOG_FORMAT, "%s - %s", timestamp, format);

    pthread_mutex_lock(m);
    ret = vfprintf(f, buffer, args);
    pthread_mutex_unlock(m);

    va_end(args);
    return ret;
}

int MakePath (const char path[], mode_t perm) 
{
    int ret, len;
    int i = 0;
    int j = 0;
    char tmp[MAX_PATH_LEN];

    len = strlen(path);

    while (i < len) {

        /* get the next folder name */
        do {             
            tmp[j++] = path[i++];
        } while (tmp[j-1] != '/' && tmp[j-1] != '\0');

        tmp[j] = '\0';

        /* try to creake dir (if it already exists it not considered an error) */
        ret = mkdir(tmp, perm);
        if (ret != 0 && errno != EEXIST) {
            return -1;
        }
    }

    return 0;
}

int DirExists (const char dir[])
{
    DIR* d = opendir(dir);
    if (d) {
        /* Directory exists */
        closedir(d);
        return 1;
    } else if (errno == ENOENT) {
        /* Directory does not exist. */
        return 0;
    }

    return -1;
}

int CreateTransmissionDir (const char dir_name[], int logger_n)
{
    char path[MAX_PATH_LEN];
    int logger2_n;

    if (logger_n == 0) {
        logger2_n = 1;
    }
    else {
        logger2_n = 0;
    }

    /* create tof dir */
    snprintf(path, MAX_PATH_LEN, "%s/voxl-logger/log%04d/run/mpa/tof", dir_name, logger_n);
    if (MakePath(path, 0777) < 0) {
        return -1;
    }

    snprintf(path, MAX_PATH_LEN, "%s/voxl-logger/log%04d/data", dir_name, logger_n);
    if (MakePath(path, 0777) < 0) {
        return -1;
    }

    snprintf(path, MAX_PATH_LEN, "%s/voxl-logger/log%04d/etc/modalai", dir_name, logger_n);
    if (MakePath(path, 0777) < 0) {
        return -1;
    }

    /* create other dir */
    snprintf(path, MAX_PATH_LEN, "%s/voxl-logger/log%04d/run/mpa/vvpx4_body_wrt_local", dir_name, logger2_n);
    if (MakePath(path, 0777) < 0) {
        return -1;
    }

    snprintf(path, MAX_PATH_LEN, "%s/voxl-logger/log%04d/data", dir_name, logger2_n);
    if (MakePath(path, 0777) < 0) {
        return -1;
    }

    snprintf(path, MAX_PATH_LEN, "%s/voxl-logger/log%04d/etc/modalai", dir_name, logger2_n);
    if (MakePath(path, 0777) < 0) {
        return -1;
    }

    return 0;
}


int RequestAllReaminingFiles (int sk, buffer_tcp *b, thread_data *t, const char dir[])
{
    char file_name[MAX_PATH_LEN];
    int ret, other_data_n;
    struct next_dir d;

    /* safely copy dir values */
    pthread_mutex_lock(&(t->lock));
    d = t->tr.d;
    pthread_mutex_unlock(&(t->lock));

    if (d.logger_n == 0) {
        other_data_n = 1;
    }
    else {
        other_data_n = 0;
    }
    
    /* request data.csv (pcd data) */
    snprintf(file_name, MAX_REL_PATH_LEN, "%s/voxl-logger/log%04d/run/mpa/tof/data.csv", d.first, d.logger_n);

    PrintLog(t->log_file, &(t->lock), "TT - Requesting %s\n", file_name);

    if ((ret = RequestFile(sk, b, file_name, dir, t)) == 0) {
        PrintLog(t->log_file, &(t->lock), "TT - Finished writing %s\n", file_name);
    }
    else {
        PrintLog(t->log_file, &(t->lock), "ERROR - TT - %d ret writing %s\n", ret, file_name);
        return -1;
    }

    /* request info.json (pcd data) */
    snprintf(file_name, MAX_REL_PATH_LEN, "%s/voxl-logger/log%04d/info.json", d.first, d.logger_n);

    PrintLog(t->log_file, &(t->lock), "TT - Requesting %s\n", file_name);

    if ((ret = RequestFile(sk, b, file_name, dir, t)) == 0) {
        PrintLog(t->log_file, &(t->lock), "TT - Finished writing %s\n", file_name);
    }
    else {
        PrintLog(t->log_file, &(t->lock), "ERROR - TT - %d ret writing %s\n", ret, file_name);
        return -1;
    }

    /* requesting data.csv (imu data) */
    snprintf(file_name, MAX_REL_PATH_LEN, "%s/voxl-logger/log%04d/run/mpa/vvpx4_body_wrt_local/data.csv", d.first, other_data_n);

    PrintLog(t->log_file, &(t->lock), "TT - Requesting %s\n", file_name);
    if ((ret = RequestFile(sk, b, file_name, dir, t)) == 0) {
        PrintLog(t->log_file, &(t->lock), "TT - Finished writing %s\n", file_name);
    }
    else {
        PrintLog(t->log_file, &(t->lock), "ERROR - TT - %d ret writing %s\n", ret, file_name);
        return -1;
    }

    /* requesting info.json (imu data) */
    snprintf(file_name, MAX_REL_PATH_LEN, "%s/voxl-logger/log%04d/info.json", d.first, other_data_n);

    PrintLog(t->log_file, &(t->lock), "TT - Requesting %s\n", file_name);
    if ((ret = RequestFile(sk, b, file_name, dir, t)) == 0) {
        PrintLog(t->log_file, &(t->lock), "TT - Finished writing %s\n", file_name);
    }
    else {
        PrintLog(t->log_file, &(t->lock), "ERROR - TT - %d ret writing %s\n", ret, file_name);
        return -1;
    }

    return 0;
}

/* a send/receive command, it returns a negative value if the send/receive fails (saving the error string in the rec buffer). It is used in order to know if the connection is still active */
int TcpSendReceiveMT (int sk, buffer_tcp *b, char send[], char rec[], thread_data *t, pthread_t id) {

    int ret;
    
    if (tcp_send(sk, send) == 0) {
        snprintf(rec, BUFSIZ+1, "ERROR, UNABLE TO WRITE\n");
        PrintLog(t->log_file, &(t->lock), rec);
        return -1;
    }

    if ((ret = tcp_readline(sk, b, rec)) == EOF || ret == 0) {
        snprintf(rec, BUFSIZ+1, "ERROR, UNABLE TO READ\n");
        PrintLog(t->log_file, &(t->lock), rec);
        return -2;
    }

    /* check if the Server has send an error message. If so, exit */
    if (strncmp(rec, "ERROR", 5) == 0) {
        PrintLog(t->log_file, &(t->lock), "R - ERROR received from Server: %s\n", rec);
        ExitMainThread(sk, t, id, EXIT_FAILURE);
    }

    /* print the result, only if it is not a STATUS response */
    if (strncmp(rec, "STATUS", 6) != 0) {
        PrintLog(t->log_file, &(t->lock), "R - %s\n", rec);
    }

    return 0;
}

void ExitMainThread (int sk, thread_data *t, pthread_t id, int exit_code)
{
    PrintLog(t->log_file, &(t->lock), "CLOSING MAIN THREAD\n");

    /* safely close the transmissione thread */
    if (t->status == 1 || t->running == 1) {
        PrintLog(t->log_file, &(t->lock), "Transmission thread is running, trying to join...\n");
        t->running = 0;
        if (pthread_join(id, NULL) != 0) {
            PrintLog(t->log_file, &(t->lock), "Failed to join transmission thread\n");
            EndIO();
            fclose(t->log_file);
            exit(EXIT_FAILURE);
        }
        PrintLog(t->log_file, &(t->lock), "Transmission thread joined\n");
    } else {
        PrintLog(t->log_file, &(t->lock), "Transmission thread is not running, nothing to join...\n");
    }

    /* end curses mode */
    EndIO();

    /* destroy the mutex */
    pthread_mutex_destroy(&(t->lock));

    printf("Closing client...\n");
    tcp_send(sk, "CLOSE\n");
    PrintLog(t->log_file, &(t->lock), "CLIENT CLOSED\n");

    fclose(t->log_file);
    exit(exit_code);
}

/* a send/receive command, it returns a negative value if the send/receive fails (saving the error string in the rec buffer). It is used in order to know if the connection is still active */
int TcpSendReceiveTT (int sk, buffer_tcp *b, char send[], char rec[], thread_data *t) {

    int ret;
    
    if (tcp_send(sk, send) == 0) {
        snprintf(rec, BUFSIZ+1, "TT - ERROR, UNABLE TO WRITE\n");
        PrintLog(t->log_file, &(t->lock), rec);
        ExitTransmissionThread(sk, t, -2);
        return -1;
    }

    if ((ret = tcp_readline(sk, b, rec)) == EOF || ret == 0) {
        snprintf(rec, BUFSIZ+1, "TT - ERROR, UNABLE TO READ\n");
        PrintLog(t->log_file, &(t->lock), rec);
        ExitTransmissionThread(sk, t, -2);
        return -2;
    }

    /* check if the Server has send an error message. If so, exit */
    if (strncmp(rec, "ERROR", 5) == 0) {
        PrintLog(t->log_file, &(t->lock), "TT - R - ERROR received from Server: %s\n", rec);
        ExitTransmissionThread(sk, t, -1);
    }

    /* print the result */
    PrintLog(t->log_file, &(t->lock), "TT - R - %s\n", rec);

    return 0;
}

void ExitTransmissionThread (int sk, thread_data *t, int exit_code)
{
    PrintLog(t->log_file, &(t->lock), "TT - CLOSING TRANSMISSION THREAD\n");

    tcp_send(sk, "CLOSE\n");
    PrintLog(t->log_file, &(t->lock), "TT - TRANSMISSION THREAD CLOSED\n");

    t->status = exit_code;
    pthread_exit(NULL);
}

int TimestampString (char timestamp[], const char format[])
{
    time_t timer;
    struct tm* tm_info;

    timer = time(NULL);
    tm_info = localtime(&timer);

    strftime(timestamp, 26, format, tm_info);

    return 0;
}

int CreateTransmissionThread (int sk, thread_data *t, pthread_t *thread_id)
{
    t->running = 1;
    PrintLog(t->log_file, &(t->lock), "Starting the transmission thread\n");
    if (pthread_create(thread_id, NULL, transmission_thread, (void *)t) != 0) {
        PrintLog(t->log_file, &(t->lock), "ERROR - Failed to create transmission thread\n");
        return -1;
    }

    PrintLog(t->log_file, &(t->lock), "Transmission thread started\n");
    return 0;
}

int Connect (char ip[], int port, int *connected, int *end, thread_data *t)
{
    int sk;

    PrintLog(t->log_file, &(t->lock), "Trying to connect to the Server\n");

    /* try to connect to the server */
    do {
        if ((sk = create_tcp_client_connection(ip, port)) < 0) {
            PrintLog(t->log_file, &(t->lock), "Unable to connect to the Server, retrying in a bit\n");
            mvprintw(2, 0, "LOGGER: DISCONNECTED");
            refresh();

            /* if the user inputs 5, the program stops trying to connect */
            if (GetUserInput() == '5') {
                *end = 1;
                break;
            }

            sleep(3);
        }
        else {
            PrintLog(t->log_file, &(t->lock), "Successfully connected to the Server\n");
            *connected = 1;
        }
    } while (!(*connected));

    return sk;
}