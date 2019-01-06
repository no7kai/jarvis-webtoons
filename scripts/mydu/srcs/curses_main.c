#include "mydu.h"
#define ALIGN_WSIZE     80
#define ALIGN_PC        90
#define ALIGN_FILES     102

/*
** diff : number of lines under the display
*/

static enum e_iter_job   curses_display_iter(size_t level, struct node *node,
    void *userdata)
{
    struct curses_cfg       *cfg = userdata;
    char                    wsize[20];
    int                     pair;
    int                     diff;

    if ((level > 1) || ((node->space.total == 0) &&
            (!(cfg->cfg->flags & FLAG_EMPTY_NODES))))
        return (STOP_NODE);
    cfg->display_index++;
    diff = (int)cfg->select_index - LINES;
    if ((int)cfg->display_index < diff + (LINES >> 1))
        return CONTINUE;
    pair = COLOR_PAIR((cfg->select == node ? COLOR_SELECTED : COLOR_DEFAULT));
    attron(pair);
    ft_wsize(node->space.total, wsize, 20);
    mvprintw(cfg->line, ALIGN_WSIZE, "%s", wsize);
    mvprintw(cfg->line, ALIGN_PC, "%4.2f%%", (node->parent) ?
        (double)node->space.total / (double)node->parent->space.total * 100.0
        : 100);
    mvprintw(cfg->line, ALIGN_FILES, "%lu", node->files.total);
    mvprintw(cfg->line, 0, "%3d %s", cfg->display_index,
        (node == cfg->node) ? node->path : node->name);
    attroff(pair);
    cfg->line++;
    if (node == cfg->node)
        mvprintw(cfg->line++, 0, "--------------------\n");
    if (cfg->line > LINES)
        return (STOP_TREE);
    return (level == 0 ? CONTINUE : STOP_NODE);
}

/*
** search for "node" into lst, each lst->content is a (struct node *)
*/

static int            lst_indexof(struct s_list *lst, const struct node *node)
{
    int      i;

    i = 0;
    while (lst)
    {
        if (node == lst->content)
            return (i);
        i++;
        lst = lst->next;
    }
    return (-1);
}

static void         curses_select(struct curses_cfg *curse, int index)
{
    struct s_list       *item;

    if (index < 0)
    {
        if (curse->node->parent)
        {
            curse->select = curse->node->parent;
            curse->select_index = 0;
            return ;
        }
        return ;
    }
    item = ft_lstat(curse->node->childs, index);
    if (!item)
        return ;
    curse->select = item->content;
    curse->select_index = (size_t)index;
}

/*
** move the current directory into the parent one.
** if no parent is available then does nothing
*/

static void         curses_updir(struct curses_cfg *curse)
{
    int         idx;
    struct node *last;

    if (!curse->node->parent)
        return ;
    last = curse->node;
    curse->node = curse->node->parent;
    curse->select = last;
    idx = lst_indexof(curse->node->childs, last);
    if (idx >= 0)
        curse->select_index = (size_t)idx;
    else
        curse->select_index = 0;
}

static inline void  curses_error_key(const int key)
{
    clear();
    mvprintw(LINES >> 1, COLS >> 1, "unknow key: %c (%d)\n", (char)key, key);
    refresh();
    getch();
}

int         main_window_draw(struct curses_window *win, void *userdata)
{
    (void)userdata;
    clear();
    node_iter(PREFIX, win->curse->node, win->curse, 0, &curses_display_iter);
    win->curse->line = 0;
    win->curse->display_index = 0;
    refresh();
    return (0);
}

static void     main_window_delete(struct curses_window *win, struct node *node)
{
    struct node     *parent;
    struct s_list   *lst;

    if (!node)
        return ;
    parent = node->parent;
    if ((!parent) || (!curses_delete(win, node)))
        return ;
    for (lst = parent->childs; lst; lst = lst->next)
    {
        if (lst->content == node)
        {
            ft_lstremove(&lst, &parent->childs, NULL);
            break ;
        }
    }
    if (win->curse->select_index > 0)
        win->curse->select_index--;
    lst = ft_lstat(parent->childs, (int)win->curse->select_index);
    win->curse->select = (!lst) ? NULL : lst->content;
}

int   main_window_input(struct curses_window *win, void *userdata, int key)
{
    struct curses_cfg   *curse = win->curse;

    (void)userdata;
    if ((key == '\n') || (key == ARROW_RIGHT))
    {
        if (curse->select == curse->node)
            curses_updir(curse);
        else if (curse->select->files.total > 0)
        {
            curse->node = curse->select;
            curse->select_index = 0;
            curse->select = (curse->node->childs) ?
                curse->node->childs->content : curse->node;
        }
    }
    else if ((key == BACKSPACE) || (key == ARROW_LEFT))
        curses_updir(curse);
    else if ((key == ARROW_DOWN) || (key == ARROW_UP))
        curses_select(curse,
            (int)curse->select_index + ((key == ARROW_UP) ? -1 : 1));
    else if (curse->cfg->flags & FLAG_VERBOSE)
        curses_error_key(key);
    else if (key == 'p')
        curses_window_info(win);
    else if (key == 'f')
        curses_files_run(win, win->curse->node);
    else if (key == 'd')
        main_window_delete(win, win->curse->select);
    return (0);
}
