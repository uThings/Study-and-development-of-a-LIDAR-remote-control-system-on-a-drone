#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "voxl-logger_lib.h"

void sleepms (int ms) 
{
    struct timespec rem, req; 
    req.tv_sec = 0;
    req.tv_nsec = ms * 1000000;
    nanosleep(&req, &rem);
    return;
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

        /* try to creake dir (if it already exists it is not considered an error) */
        ret = mkdir(tmp, perm);
        if (ret != 0 && errno != EEXIST) {
            return -1;
        }
    }

    return 0;
}

int CopyFile (const char in[], const char out[])
{
    FILE *f_in, *f_out;
    char buffer[MAX_LINE_LEN];

    if((f_in = fopen (in, "r" )) == NULL) {
        return -1;
    }

    if((f_out = fopen (out, "w+" )) == NULL) {
        return -2;
    }

    /* copies the file from the reference data line by line */
    while (fgets(buffer, MAX_LINE_LEN-1, f_in) != NULL) {
        fputs(buffer, f_out);
    }

    fclose(f_in);
    fclose(f_out);

    return 0;
}

int CopyOtherFiles (const char timestamp[], int logger_n) 
{
    
    char in_path[MAX_PATH_LEN];
    char out_path[MAX_PATH_LEN];
    int other_n;

    /* set other data position */
    if (logger_n == 0) {
        other_n = 1;
    }
    else {
        other_n = 0;
    }

    /* copy data.csv (pcd data) */
    snprintf(in_path, MAX_PATH_LEN, "%s/pcd_data/data.csv", SOURCE_DIR);
    snprintf(out_path, MAX_PATH_LEN, "%s/%s/voxl-logger/log%04d/run/mpa/tof/data.csv", OUT_DIR, timestamp, logger_n);
    if (CopyFile(in_path, out_path) < 0) {
        return -1;
    }

    /* copy info.json (pcd data) */
    snprintf(in_path, MAX_PATH_LEN, "%s/pcd_data/info.json", SOURCE_DIR);
    snprintf(out_path, MAX_PATH_LEN, "%s/%s/voxl-logger/log%04d/info.json", OUT_DIR, timestamp, logger_n);
    if (CopyFile(in_path, out_path) < 0) {
        return -1;
    }

    /* copy data.csv (imu data) */
    snprintf(in_path, MAX_PATH_LEN, "%s/other_data/data.csv", SOURCE_DIR);
    snprintf(out_path, MAX_PATH_LEN, "%s/%s/voxl-logger/log%04d/run/mpa/vvpx4_body_wrt_local/data.csv", OUT_DIR, timestamp, other_n);
    if (CopyFile(in_path, out_path) < 0) {
        return -1;
    }

    /* copy info.json (imu data) */
    snprintf(in_path, MAX_PATH_LEN, "%s/other_data/info.json", SOURCE_DIR);
    snprintf(out_path, MAX_PATH_LEN, "%s/%s/voxl-logger/log%04d/info.json", OUT_DIR, timestamp, other_n);
    if (CopyFile(in_path, out_path) < 0) {
        return -1;
    }

    return 0;
}