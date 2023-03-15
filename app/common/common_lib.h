#ifndef COMMON_LIB
#define COMMON_LIB

#include <stdio.h>


#define TIMESTAMP_LEN 16

typedef struct buffer_tcp {
    int dim;
    int i;
    char buffer[BUFSIZ+1];
} buffer_tcp;

struct next_file
{
    int first;
    int last;
    int count;
};

struct next_dir
{
    char first[TIMESTAMP_LEN];
    char last[TIMESTAMP_LEN];
    int count;
    int logger_n;
};

struct tr_info
{
    struct next_file f;
    struct next_dir d;
};

void sleepms (int ms);
int tcp_getchar_bs (int sk, buffer_tcp* b);
buffer_tcp init_buffer_tcp ();
int tcp_readline (int csk, buffer_tcp* b, char buf[]);

#endif