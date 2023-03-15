#ifndef IO_LIB
#define IO_LIB

#include "../common/common_lib.h"

void InitIO ();
void EndIO ();
void print_command_info (int y, int x, int refresh_flag);
void PrintInfo (int logger_status, int thread_status, struct tr_info tr);
char GetUserInput ();

#endif
