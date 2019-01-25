#include "mydu.h"
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>

static int  curses_files_cmp(struct s_list *a, struct s_list *b)
{
    const struct file_entry *fa = a->content;
    const struct file_entry *fb = b->content;

    if (fa->st.st_blocks < fb->st.st_blocks)
        return (1);
    else if (fa->st.st_blocks > fb->st.st_blocks)
        return (-1);
    return (ft_strcmp(fa->name, fb->name));
}

static int  curses_files_draw(struct curses_window *win)
{
    int                 pair;
    int                 line;
    struct s_list       *lst;
    struct files_window *files = win->userdata;
    struct file_entry   *file;

    line = 3;
    lst = files->content;
    while ((lst) && (line < win->h))
    {
        file = lst->content;
        lst = lst->next;
        pair = COLOR_PAIR(files->selected == file ? COLOR_SELECTED : COLOR_DEFAULT);
        attron(pair);
        mvprintw(win->y + line, win->x + 2, "%s", file->name);
        mvprintw(win->y + line, win->x + win->w - 9, "%s", file->wsize);
        attroff(pair);
        line++;
    }
    return (EXIT_SUCCESS);
}

static struct file_entry *curses_files_mkentry(const char *path,
    const char *name, const size_t blksize)
{
    struct file_entry   *entry;

    entry = ft_memalloc(sizeof(struct file_entry));
    if (!entry)
        return (NULL);
    ft_strcpy(entry->name, name);
    if (stat(path, &entry->st) >= 0)
        ft_wsize((size_t)entry->st.st_blocks * blksize, entry->wsize, WSIZE_LEN);
    else if ((entry->st.st_mode & S_IFDIR) || (!(entry->st.st_mode & S_IFREG)))
    {
        free(entry);
        return (NULL);
    }
    else
        ft_strcpy(entry->wsize, "???");
    return (entry);
}

static int  curses_files_init(struct curses_window *win)
{
    struct files_window         *files = win->userdata;
	DIR				            *dir;
	struct dirent               *ent;
    struct file_entry           *entry;
    char                        path[PATH_MAX];

    files->content = NULL;
    dir = opendir(files->node->path);
    if (!dir)
    {
        // TODO : display error window here
        win->flags |= WIN_QUIT;
        return (-1);
    }
    statfs(files->node->path, &files->fs);
    while ((ent = (readdir(dir))) != NULL)
    {
        if ((!ft_strcmp(ent->d_name, ".")) || (!ft_strcmp(ent->d_name, "..")))
            continue ;
        if (ent->d_type & DT_DIR)
            continue ;
        ft_snprintf(path, PATH_MAX, "%s/%s", files->node->path, ent->d_name);
        entry = curses_files_mkentry(path, ent->d_name,
            (size_t)(files->fs.f_bsize >> 3));
        if (entry)
            ft_lstpush_sort(&files->content,
                ft_lstnewlink(entry, 0), &curses_files_cmp);
    }
    closedir(dir);
    if (files->content)
        files->selected = files->content->content;
    return (0);
}

static int  curses_files_quit(struct curses_window *win)
{
    struct files_window     *files = win->userdata;

    ft_lstdel(&files->content, &ft_lstpulverisator);
	return (0);
}

static void curses_files_select(struct files_window *files, const int direction)
{
    struct s_list   *item;

    if ((direction < 0) && (files->selected_index == 0))
        return ;
    item = ft_lstat(files->content, (int)files->selected_index + direction);
    if (item)
    {
        files->selected = item->content;
        if (direction < 0)
            files->selected_index--;
        else
            files->selected_index++;
    }
}

static void curses_files_delete(struct files_window *files, struct file_entry *file)
{
    struct s_list   *item;
    char            path[PATH_MAX];

    if (!file)
        return ;
    item = lst_search_content(files->content, files->selected);
    if (!item)
        return ;
    ft_snprintf(path, PATH_MAX, "%s/%s", files->node->path, file->name);
    if (unlink(path) < 0)
        return ;
    files->selected = (item->next) ? item->next->content : NULL;
    ft_lstremove(&item, &files->content, NULL);
}

static int  curses_files_input(struct curses_window *win, int key)
{
    struct files_window         *files = win->userdata;

    if (key == ARROW_DOWN)
        curses_files_select(files, 1);
    else if (key == ARROW_UP)
        curses_files_select(files, -1);
    else if ((key == 'd') && (curses_confirm(win, "Delete selected file ?", false)))
        curses_files_delete(files, files->selected);
    else if ((key == 'i') && (files->selected))
	{
        curses_filefinfo(win, files->selected);
		win->parent->draw(win->parent);
	}
    else if (key == 'u')
    {
        win->quit(win);
        win->init(win);
    }
    return (0);
}

void        curses_files_run(struct curses_window *win, struct node *node)
{
    struct curses_window        this;
    struct files_window         files;

    ft_bzero(&files, sizeof(struct files_window));
    files.node = node;
    ft_snprintf(files.title, PATH_MAX, "%s%s", "Content of ", node->name);
    this = (struct curses_window) {
        .parent = win,
        .title = files.title,
        .draw = &curses_files_draw,
        .input = &curses_files_input,
        .init = &curses_files_init,
        .quit = &curses_files_quit,
        .userdata = &files
    };
    curses_new_window(curses_centerfrom_parent(&this, 100, LINES - 16));
}
