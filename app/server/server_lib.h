#ifndef SERVER_LIB
#define SERVER_LIB

#include <ftw.h>
#include "../common/common_lib.h"

#define MAX_PATH_LEN 256
#define MAX_REL_PATH_LEN MAX_PATH_LEN/2
#define BUFFER_DIM 256
#define DATA_DIR "../../data_generation/generated_data"

int ProcNumber (const char cmd[]); 
int Status (int csk, const char logger_name[]);
int Start (int csk, const char cmd[], const char start_name[], const char logger_name[]); 
int Stop (int csk, const char start_name[], const char logger_name[]);
int SendNext (int csk, const char logger_name[], const char directory[]);
int FindNextTr (struct tr_info *tr, const char dir_name[]);
int FindNextFile (struct next_file *f, const char path[]);
int FindLidarDataInfo(struct next_dir *d, const char dir_name[]);
int FindFirstDir (struct next_dir *d, const char dir_name[]);
time_t StringToTime (const char s[]);
int DirExists (const char dir[]);
int ExtractFileNumber (const char filename[]);
int RemoveFile (int csk, const char cmd[], const char directory[]); 
int SendFile (int csk, const char cmd[]);
int CompressAndSend (int csk, const char file_path[], const char buf[]);
int RemoveDir (int csk, const char cmd[], const char dir_name[]);
struct FTW;
int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
int rmrf(char *path);

#endif