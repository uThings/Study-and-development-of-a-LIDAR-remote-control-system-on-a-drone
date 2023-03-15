#ifndef CLIENT_LIB
#define CLIENT_LIB

#include <stdatomic.h>
#include "../common/common_lib.h"

#define MAX_PATH_LEN 256
#define MAX_REL_PATH_LEN MAX_PATH_LEN/2
#define MAXCMDLEN 128
#define DATA_DIR "rec_data"
#define WAIT_DELAY_MS 100

#define MAX_LOG_FORMAT 256

#define IPLEN 64

typedef struct thread_data {
    atomic_int status;
    atomic_int running;
    char ip[IPLEN];
    int port;
    pthread_mutex_t lock;
    struct tr_info tr;
    FILE *log_file;
} thread_data;

int tcp_binary_getchar_bs (int sk, buffer_tcp* b, thread_data *t);
void *transmission_thread (void* arg);
int ReceiveAndDecompress (int sk, thread_data *t, buffer_tcp* b, const char file_path[], long int rec_len, long int *orig_len);
int RequestFile (int sk, buffer_tcp* b, const char rel_path[], const char data_dir[], thread_data *t);
int PrintLog (FILE *f, pthread_mutex_t *m, const char format[], ...);
int MakePath (const char path[], mode_t perm);
int DirExists (const char dir[]);
int CreateTransmissionDir (const char dir_name[], int logger_n);
int TimestampString (char timestamp[], const char format[]);
int RequestAllReaminingFiles (int sk, buffer_tcp *b, thread_data *t, const char dir[]);
int TcpSendReceiveMT (int sk, buffer_tcp *b, char send[], char rec[], thread_data *t, pthread_t id);
void ExitMainThread (int sk, thread_data *t, pthread_t id, int exit_code);
int TcpSendReceiveTT (int sk, buffer_tcp *b, char send[], char rec[], thread_data *t);
void ExitTransmissionThread (int sk, thread_data *t, int exit_code);
int CreateTransmissionThread (int sk, thread_data *t, pthread_t *thread_id);
int Connect (char ip[], int port, int *connected, int *end, thread_data *t);

#endif