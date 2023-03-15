#define _XOPEN_SOURCE 700  /* for strptime */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h> 
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "server_lib.h"
#include "../common/compression_lib.h"
#include "tcpsocketlib.h"

/* returns the number of process (-1 on error) with a certain name that are 
   running on the system */
int ProcNumber (const char name[]) 
{
    FILE *res;
    char buf[BUFFER_DIM];
    int cnt = 0;

    snprintf(buf, BUFFER_DIM-9, "pgrep -l %s", name);
    if ((res = popen(buf, "r")) == NULL) {
        return -1;
    }

    while (fgets(buf, BUFFER_DIM-1, res) != 0) {
        cnt++;
    }

    pclose(res);

    return cnt;
}

int Status (int csk, const char logger_name[])
{
    int n = ProcNumber(logger_name);
    if (n == 0) {
        tcp_send(csk, "STATUS: 0\n");
    }
    else if (n > 0) {
        tcp_send(csk, "STATUS: 1\n");
    }
    else {
        tcp_send(csk, "ERROR FINDING ACTIVE PROCESSES\n");
        return -1;
    }

    return 0;
}

int Start (int csk, const char cmd[], const char start_name[], const char logger_name[]) 
{
    int n;
    char voxl_command[128];
    char timestamp[TIMESTAMP_LEN];

    sscanf(cmd, "%*s %s", timestamp);
    snprintf(voxl_command, 128, "%s %s", start_name, timestamp);

    if ((n = ProcNumber(logger_name)) == 0) {
        system(voxl_command);
        printf("Starting the logging system\n");
        tcp_send(csk, "Logging system STARTED\n");
    } 
    else if (n > 0) {
        printf("Logging system is already active\n");
        tcp_send(csk, "The logging system is already running\n");
    }
    else {
        printf("Error finding active processes\n");
        tcp_send(csk, "ERROR FINDING ACTIVE PROCESSES\n");
        return -1;
    }

    return 0;
}

int Stop (int csk, const char stop_name[], const char logger_name[]) 
{
    int n;

    if ((n = ProcNumber(logger_name)) > 0) {
        system(stop_name);
        printf("Stopping the logging system\n");
        tcp_send(csk, "Logging system STOPPED\n");
    } 
    else if (n == 0){
        printf("Logging system is NOT active\n");
        tcp_send(csk, "The logging system is NOT running\n");
    }
    else {
        printf("Error finding active processes\n");
        tcp_send(csk, "ERROR FINDING ACTIVE PROCESSES\n");
        return -1;
    }

    return 0;
}

int SendNext (int csk, const char logger_name[], const char dir_name[])
{
    struct tr_info tr;
    char buf[BUFSIZ+1];
    char file_name[MAX_PATH_LEN];
    FILE *f;
    int logger_state;
    int min_count;
    int ret;

    Bytef* comp_buffer = NULL;
    long int source_len;
    long int comp_len;

    /* find the next dir and file */
    ret = FindNextTr(&tr, dir_name);
    if (ret == -1) {
        fprintf(stderr, "Error finding the next dir\n");
        tcp_send(csk, "ERROR FINDING NEXT DIR\n");
        return -1;
    }
    else if (ret == -2) {
        fprintf(stderr, "Error finding the next file\n");
        tcp_send(csk, "ERROR FINDING NEXT FILE\n");
        return -1;
    }

    /* check the logger_state */
    if ((logger_state = ProcNumber(logger_name)) < 0) {
        fprintf(stderr, "Error finding active processes\n");
        tcp_send(csk, "ERROR FINDING ACTIVE PROCESSES\n");
        return -1;
    }

    if (logger_state && tr.d.count == 1) 
        min_count = 2;
    else
        min_count = 1;

    if (tr.d.count == 0) {
        printf("No transmission folders found, sending NO_TR\n");
        tcp_send(csk, "NO_TR\n");
    }
    else if (tr.f.count >= min_count) {
        snprintf(buf, BUFSIZ+1, "SENDING %s %d %d %d %d", tr.d.first, tr.d.logger_n, tr.f.first, tr.f.last, tr.f.count);
        snprintf(file_name, MAX_PATH_LEN, "%s/%s/voxl-logger/log%04d/run/mpa/tof/data_%d.pcd", dir_name, tr.d.first, tr.d.logger_n, tr.f.first);
        printf("Sending %s\n", file_name);

        if ((ret = CompressAndSend(csk, file_name, buf)) < 0) {
            fprintf(stderr, "Error compressing buffer\n");
            return -1;
        }

        printf("Finished sending %s\n", file_name);
        return 0;
    }
    else if (logger_state && tr.d.count == 1) {
        /* there is only one file that is being currently written */
        printf("Sending WAIT\n");
        tcp_send(csk, "WAIT\n"); 
    }
    else {
        /* there are no files (and the logger is inactive) */
        printf("Sending EMPTY\n");
        snprintf(buf, BUFSIZ+1, "EMPTY %s %d\n", tr.d.first, tr.d.logger_n);
        tcp_send(csk, buf);
    }

    return 0;
}

int CompressAndSend (int csk, const char file_path[], const char buf[])
{
    char send_buf[BUFSIZ+1];
    Bytef *comp_buffer = NULL;
    long int source_len = 0;
    long int comp_len = 0;

    /* compress the file */
    if (compressor(file_path, &source_len, &comp_buffer, &comp_len) < 0) {
        free(comp_buffer);
        tcp_send(csk, "ERROR compressing buffer\n");
        return -1;
    }

    snprintf(send_buf, BUFSIZ+1, "%s %ld %ld\n", buf, source_len, comp_len);
    tcp_send(csk, send_buf);

    /* send the file */
    tcp_binary_send (csk, comp_buffer, comp_len);
    free(comp_buffer);
    tcp_send(csk, "END\n");
    return 0;
}

int FindNextTr (struct tr_info *tr, const char dir_name[])
{
    char path[MAX_PATH_LEN];

    /* find the first lidar data directory and the
       lidar data sub-dir (can be either in log0000 or in log0001) */
    if (FindLidarDataInfo(&tr->d, dir_name) < 0) {
        return -1;
    }

    snprintf(path, MAX_PATH_LEN, "%s/%s/voxl-logger/log%04d/run/mpa/tof", dir_name, tr->d.first, tr->d.logger_n);

    if (FindNextFile(&tr->f, path) < 0) {
        return -2;
    }
}

int FindNextFile (struct next_file *f, const char path[])
{
    DIR *d;
    struct dirent *dir;
    int tmp;

    d = opendir(path);

    if (d) {
        f->count = 0;
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, "data.csv") != 0 &&
                strncmp(dir->d_name, ".", 1) != 0 &&
                strncmp(dir->d_name, "..", 2) != 0) {
                    
                if (f->count == 0) {
                    /* save the first file number as the temporary minimum (and maximum) */
                    f->first = ExtractFileNumber(dir->d_name);
                    f->last = f->first;
                }
                else {
                    tmp = ExtractFileNumber(dir->d_name);
                    if (tmp < f->first) {
                        f->first = tmp;
                    }

                    if (tmp > f->last) {
                        f->last = tmp;
                    }
                }
                
                f->count++;
            }
        }
        closedir (d);
    }

    return 0;
}

int FindLidarDataInfo(struct next_dir *nd, const char dir_name[])
{
    char path[MAX_PATH_LEN];

    /* find the oldest dir (minor name which is the timestamp) */
    FindFirstDir(nd, dir_name);

    if (nd->count == 0)
        return 0;

    snprintf(path, MAX_PATH_LEN, "%s/%s/voxl-logger/log0000/run/mpa/tof", dir_name, nd->first);
    if (DirExists(path) == 1) {
        nd->logger_n = 0;
        return 0;
    }

    snprintf(path, MAX_PATH_LEN, "%s/%s/voxl-logger/log0001/run/mpa/tof", dir_name, nd->first);

    if (DirExists(path) == 1) {
        nd->logger_n = 1;
        return 0;
    }

    return -1;
}

int FindFirstDir (struct next_dir *nd, const char dir_name[])
{
    DIR *d;
    struct dirent *dir;
    time_t ts_first, ts_last, ts_tmp;

    d = opendir(dir_name);

    if (d) {
        nd->count = 0;
        while ((dir = readdir(d)) != NULL) {
            if (strncmp(dir->d_name, ".", 1) != 0 &&
                strncmp(dir->d_name, "..", 2) != 0) {
                    
                if (nd->count == 0) {
                    /* save the first file number as the temporary minimum (and maximum) */
                    ts_first = StringToTime(dir->d_name);
                    strncpy(nd->first, dir->d_name, TIMESTAMP_LEN);
                    ts_last = ts_first;
                    strncpy(nd->last, dir->d_name, TIMESTAMP_LEN);
                }
                else {
                    ts_tmp = StringToTime(dir->d_name);
                    if (ts_tmp < ts_first) {
                        ts_first = ts_tmp;
                        strncpy(nd->first, dir->d_name, TIMESTAMP_LEN);
                    }

                    if (ts_tmp > ts_last) {
                        ts_last = ts_tmp;
                        strncpy(nd->last, dir->d_name, TIMESTAMP_LEN);
                    }
                }
                
                nd->count++;
            }
        }
        closedir (d);
    }

    return 0;
}   

time_t StringToTime (const char s[])
{
    struct tm tm;
    if (strptime(s, "%Y%m%d-%H%M%S", &tm) != NULL) {
        return mktime(&tm);
    }
    
    return -1;
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

int ExtractFileNumber (const char filename[])
{
    int n;
    sscanf(filename, "%*[^_]_%d.%*s", &n);
    return n;
}

int RemoveFile (int csk, const char cmd[], const char dir_name[]) 
{
    int file_number;
    int logger_n;
    char file_name[MAX_PATH_LEN];
    char timestamp[TIMESTAMP_LEN];
    char buf[BUFSIZ+1];

    sscanf(cmd, "%*s %s %d %d", timestamp, &logger_n, &file_number);
    snprintf(file_name, MAX_PATH_LEN, "%s/%s/voxl-logger/log%04d/run/mpa/tof/data_%d.pcd", dir_name, timestamp, logger_n, file_number);
    printf("Removing %s\n", file_name);

    /* removes the file */
    if (remove(file_name) != 0) {
        tcp_send(csk, "ERROR DELETING FILE\n");
        return -1;
    }

    sprintf(buf, "FILE %d, %s REMOVED\n", file_number, timestamp);
    tcp_send(csk, buf);
    return 0;
}

int SendFile (int csk, const char cmd[])
{
    char buf[BUFSIZ+1];
    char rel_path[MAX_PATH_LEN/2];
    char file_path[MAX_PATH_LEN];
    int ret;

    sscanf(cmd, "%*s %s", rel_path);
    snprintf(file_path, MAX_PATH_LEN, "%s/%s", DATA_DIR, rel_path);
    snprintf(buf, BUFSIZ+1, "SENDING REQUESTED %s", file_path);

    if ((ret = CompressAndSend(csk, file_path, buf)) < 0) {
        fprintf(stderr, "ERROR compressing buffer\n");
        return -1;
    }

    printf("Finished sending %s\n", file_path);
    return 0;
}

int RemoveDir (int csk, const char cmd[], const char dir_name[])
{
    char path[MAX_PATH_LEN];
    char timestamp[TIMESTAMP_LEN];
    char buf[BUFSIZ+1];

    sscanf(cmd, "%*s %s", timestamp);
    snprintf(path, MAX_PATH_LEN, "%s/%s", dir_name, timestamp);

    if (rmrf(path) < 0) {
        snprintf(buf, BUFSIZ+1, "ERROR REMOVING DIR %s\n", timestamp);
        tcp_send(csk, buf);
        return -1;
    }

    snprintf(buf, BUFSIZ+1, "DIR %s REMOVED\n", timestamp);
    tcp_send(csk, buf);

    return 0;
}

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

int rmrf(char *path)
{
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}
