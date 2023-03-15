#define SOURCE_DIR "reference_data"
#define OUT_DIR "generated_data"
#define MAX_LINE_LEN 512
#define MAX_PATH_LEN 512
#define TIMESTAMP_LEN 26

void sleepms (int ms);
int MakePath (const char path[], mode_t perm);
int CopyFile (const char in[], const char out[]);
int CopyOtherFiles (const char timestamp[], int logger_n);
