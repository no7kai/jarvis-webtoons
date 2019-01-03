#include "mydu.h"
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

enum e_iter_job	node_iter(const size_t mode,
	struct node *node,
	void *userdata,
	size_t level,
	enum e_iter_job (*f)(size_t, struct node *, void *))
{
	struct s_list		*lst;
	enum e_iter_job		ret;

	if (mode & PREFIX) 
		f(level, node, userdata);
	for (lst = node->childs; lst; lst = lst->next)
	{
		ret = node_iter(mode, lst->content, userdata, level + 1, f);
		if (ret == STOP_NODE)
			return (CONTINUE);
		if (ret == STOP_TREE)
			return (STOP_TREE);
	}
	if (mode & SUFFIX)
		f(level, node, userdata);
	return (CONTINUE);
}

enum e_iter_job	node_iter_clean(size_t level, struct node *node, void *unused)
{
	(void)level;
	(void)unused;
	if (node->childs)
		ft_lstdel(&node->childs, NULL);
	free(node);
	return (CONTINUE);
}

static inline int   node_init(struct node *node, struct node *parent)
{
	if (!node)
	{
		ft_dprintf(STDERR_FILENO, "%s", "Error: out of memory\n");
		return (EXIT_FAILURE);
	}
	ft_bzero(node, sizeof(*node));
	node->parent = parent;
	return (EXIT_SUCCESS);
}

static inline void  node_update_parent(struct node *node)
{
	node->space.total += node->space.local;
	node->files.total += node->files.local;
	if (node->parent)
	{
		node->parent->files.total += node->files.total;
		node->parent->space.total += node->space.total;
	}
}

static inline void	node_walk_loop(struct node *node,
	const struct dirent *ent, struct stat *st,
	const struct config *cfg)
{
	struct node     *newnode;

	if (lstat(node->path, st) < 0)
	{
		if (cfg->flags & FLAG_VERBOSE)
			ft_dprintf(STDERR_FILENO, "failed to stat: %s: %s\n",
				node->path, strerror(errno));
		return ;
	}
	if (st->st_mode & S_IFDIR)
	{
		newnode = node_walk(node->path, node, cfg);
		if (!newnode)
			return ;
		ft_snprintf(newnode->name, FILENAME_MAXLEN, "%s", ent->d_name);
		ft_lstpush_sort(&node->childs, ft_lstnewlink(newnode, 0), cfg->sorter);
	}
	else if (st->st_mode & S_IFREG)
	{
		if (cfg->flags & FLAG_BLOCKS)
			node->space.local += (size_t)st->st_blocks * BLK_SIZE;
		else if (st->st_size > 0)
			node->space.local += (size_t)st->st_size;
		node->files.local += 1;
	}
}

__attribute((pure))
struct node  *node_walk(const char *path, struct node *parent,
	const struct config *cfg)
{
	struct node     *node;
	struct dirent   *ent;
	struct stat     st;
	DIR             *dir;

	if (node_init(node = malloc(sizeof(struct node)), parent) != EXIT_SUCCESS)
		return (NULL);
	if (!parent)
		ft_strcpy(node->name, &path[1]);
	dir = opendir(path);
	if (!dir)
	{
		if ((!parent) || (cfg->flags & FLAG_VERBOSE))
			ft_dprintf(STDERR_FILENO, "failed to opendir: %s: %s\n", path, strerror(errno));
		free(node);
		return NULL;
	}
	while ((ent = readdir(dir)) != NULL)
	{
		if (ent->d_name[0] == '.')
			continue ;
		ft_snprintf(node->path, PATH_MAX, "%s/%s", path, ent->d_name);
		node_walk_loop(node, ent, &st, cfg);
	}
	closedir(dir);
	node_update_parent(node);
	node->path_len = ft_strlen(path);
	ft_strcpy(node->path, path);
	return node;
}
