#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include "voxl-logger_lib.h"

#define DELAY_MS 55

static volatile int running = 1;

void SigHandler(int dummy)
{
    running = 0;
}

int main (int argc, char *argv[])
{
    char in_path[MAX_PATH_LEN];
    char out_path[MAX_PATH_LEN];
    int cnt = 0;
    char timestamp[TIMESTAMP_LEN];
    int r, pcd_data_n, other_data_n;
    int opt;

    while ((opt = getopt(argc, argv, "d:")) != -1) {
        switch (opt) {
            case 'd': snprintf(timestamp, TIMESTAMP_LEN, "%s", optarg); break;
            case '?': printf("Must provide the -d option with a timestamp (es: 20230102-083000)\n"); return EXIT_FAILURE;
            default: return EXIT_FAILURE;
        }
    }

    if (optind == 1) { 
        printf("Must provide the -d option with a timestamp (es: 20230102-083000)\n");  return EXIT_FAILURE;
    }

    struct sigaction s_handler;
    s_handler.sa_handler = SigHandler;
    sigaction(SIGINT, &s_handler, NULL);
    sigaction(SIGQUIT, &s_handler, NULL);
    sigaction(SIGKILL, &s_handler, NULL);
    sigaction(SIGTERM, &s_handler, NULL);

    srand(time(NULL)); 

    /* generate the folder that contains pcd data */
    r = rand() % 100;
    if (r > 50) {
        pcd_data_n = 1;
        other_data_n = 0;
    }
    else {
        pcd_data_n = 0;
        other_data_n = 1;
    }

    snprintf(out_path, MAX_PATH_LEN, "%s/%s/voxl-logger/log%04d/run/mpa/tof", OUT_DIR, timestamp, pcd_data_n);
    if (MakePath(out_path, 0777) != 0) {
        fprintf(stderr, "Error creating path\n");
        return EXIT_FAILURE;
    }

    snprintf(out_path, MAX_PATH_LEN, "%s/%s/voxl-logger/log%04d/run/mpa/vvpx4_body_wrt_local", OUT_DIR, timestamp, other_data_n);
    if (MakePath(out_path, 0777) != 0) {
        fprintf(stderr, "Error creating path\n");
        return EXIT_FAILURE;
    }

    /* generation */
    while (running) {

        snprintf(in_path, MAX_PATH_LEN, "%s/pcd_data/data_%d.pcd", SOURCE_DIR, cnt);
        snprintf(out_path, MAX_PATH_LEN, "%s/%s/voxl-logger/log%04d/run/mpa/tof/data_%d.pcd", OUT_DIR, timestamp, pcd_data_n, cnt);
    
        if (access(in_path, F_OK) != 0) {
            sleep(1);
            continue;
        }

        if (CopyFile(in_path, out_path) < 0) {
            fprintf(stderr, "Unable to copy the file %s\n", in_path);
            return EXIT_FAILURE;
        }
        
        cnt++;

        sleepms(DELAY_MS);
    }

    printf("Finished\n");

    if (CopyOtherFiles(timestamp, pcd_data_n) < 0) {
        fprintf(stderr, "Failed to copy other files\n");
        return EXIT_FAILURE;
    }

    printf("\nCLOSING, written %d files\n", cnt);

    return EXIT_SUCCESS;
}