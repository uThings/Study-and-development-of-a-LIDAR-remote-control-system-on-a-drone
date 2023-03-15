#include "io_lib.h"

#include <ncurses.h>
#include "../common/common_lib.h"

void InitIO ()
{
    /* start curses mode */
    initscr();

    noecho();
    nodelay(stdscr, TRUE);

    printw("LIDAR Control System\n\n");
    print_command_info(5, 0, 0);
    refresh();
}

void EndIO ()
{
    endwin();
}

void print_command_info (int y, int x, int refresh_flag)
{
    mvprintw(y, x, "Available Commands:\n");
    printw("(1) : Start the logging system\n\
            \r(2) : Stop the logging system\n\
            \r(3) : Start the transmission\n\
            \r(4) : Stop the transmission\n\
            \r(5) : Quit");
    
    if (refresh_flag)
        refresh();
    
    return;
}

void PrintInfo (int logger_status, int thread_status, struct tr_info tr)
{
    move(2, 0);
    clrtoeol();
    refresh();

    if (logger_status) {
        mvprintw(2, 0, "LOGGER: ON");
    }
    else {
        mvprintw(2, 0, "LOGGER: OFF");
    }

    if (thread_status == 1) {
        move(3, 0);
        clrtoeol();
        mvprintw(3, 0, "TRANSMISSION:    %s    %d/%d", tr.d.first, tr.f.first, tr.f.last);
    }
    else {
        move(3, 0);
        clrtoeol();
        mvprintw(3, 0, "TRANSMISSION: OFF");
    }

    refresh();
}

char GetUserInput ()
{
    char ch;

    /* get command */
    if ((ch = getch()) != ERR) {
        return ch;
    }

    return EOF;
}