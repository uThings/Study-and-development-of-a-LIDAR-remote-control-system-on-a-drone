#include "common_lib.h"

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "tcpsocketlib.h"

void sleepms (int ms) 
{
    struct timespec rem, req; 
    req.tv_sec = 0;
    req.tv_nsec = ms * 1000000;
    nanosleep(&req, &rem);
    return;
}

int tcp_getchar_bs (int sk, buffer_tcp* b)
{
   if (b->i >= b->dim || b->buffer[b->i] == '\0')
   {
      /* reload the buffer */
      if ((b->dim = tcp_receive (sk, b->buffer)) == -1)
         return EOF;
      b->i = 0;
   }

   return b->buffer[b->i++];
}

buffer_tcp init_buffer_tcp () 
{
    buffer_tcp b;
    b.dim = 0;
    b.i = 0;

    return b;
}

int tcp_readline (int csk, buffer_tcp* b, char buf[])
{
    char ch;
    int i = 0;
    while ((ch = tcp_getchar_bs(csk, b)) != '\n' && ch != '\0') {
        if (ch != EOF) {
            buf[i++] = ch;
        } else {
            return EOF;
        }
    }
    buf[i] = '\0';

    return i;
}