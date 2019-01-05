#include "mydu.h"

void                curses_puts_center(struct curses_window *win, const int line,
    const char *text, const size_t len)
{
    mvprintw(
        win->x + line,
        win->y + 1 + (win->w / 2) - ((int)len / 2),
        "%s",
        text);
}

void                curses_window_decorate(struct curses_window *win)
{
    if (!(win->flags & WIN_NOBORDER))
        curses_box(win->x, win->y, win->w, win->h);
    if (win->title)
        curses_puts_center(win, 1, win->title, ft_strlen(win->title));
}

int                 curses_new_window(struct curses_window *win, void *userdata)
{
    int             ret;
    int             key;

    do
    {
        curses_window_decorate(win);
        if (win->draw)
        {
            ret = win->draw(win, userdata);
            if (ret)
                return (ret);
            if (win->flags & WIN_QUIT)
                return (0);
        }
        else
            refresh();
        key = getch();
        if (win->input)
            win->input(win, userdata, key);
        if ((win->flags & WIN_CONFIRM_CLOSE) && (key == 'q') &&
                (!curses_confirm("Quit ?", false)))
            key = 0;
    }
    while ((key != 'q') || (win->flags & WIN_NOQ));
    return (0);
}

void         curses_refresh_parents(struct curses_window *win)
{
    if (!win)
        return ;

    if (win->parent)
        curses_refresh_parents(win->parent);
    curses_window_decorate(win);
    if (win->draw)
        win->draw(win, NULL);
}

static int          curses_window_info_input(struct curses_window *win,
    void *userdata, int key)
{
    (void)userdata;
    if (key == 'p')
    {
        curses_window_info(win);
        curses_refresh_parents(win);
    }
    return (0);
}

void                curses_window_info(struct curses_window *win)
{
    struct curses_window    info;

    info = (struct curses_window){
        .parent = win,
        .x = (int)((win->x == 0) ? win->h >> 2 : win->x + 3),
        .y = (int)((win->y == 0) ? win->w >> 2 : win->y + 3),
        .w = 80,
        .h = 10,
        .title = "Window information",
        .curse = win->curse,
        .input = curses_window_info_input
    };
    curses_new_window(&info, NULL);
}
