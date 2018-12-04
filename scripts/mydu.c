#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include "libft.h"

#define FLAG_FULLPATH_DISPLAY	(1u << 0)
#define FLAG_LOCALSTAT			(1u << 1)
#define FLAG_REVERSE			(1u << 2)

struct config {
	const char		*root;
	size_t			flags;
	size_t			path_len_align;
};

struct node {
	struct node     *parent;
	struct s_list   *childs;
	char            path[PATH_MAX];
	size_t			path_len;
	size_t          space;
	size_t          total_space;
	size_t          files;
	size_t          total_files;
};

static int      node_cmp(struct node *a, struct node *b)
{
	if (a->total_space < b->total_space)
		return (1);
	else if (a->total_space > b->total_space)
		return (-1);
	return (ft_strcmp(a->path, b->path));
}

static int      lst_cmp(t_list *a, t_list *b)
{
	return (node_cmp(a->content, b->content));
}

static int		lst_revcmp(t_list *a, t_list *b)
{
	return (-node_cmp(a->content, b->content));
}

static inline void  node_update_parent(struct node *node)
{
	if (node->parent)
	{
		node->parent->total_files += node->total_files;
		node->parent->total_space += node->total_space;
	}
}

static inline int   node_init(struct node *node, struct node *parent)
{
	if (!node)
		return (EXIT_FAILURE);
	ft_bzero(node, sizeof(*node));
	node->parent = parent;
	return (EXIT_SUCCESS);
}

static struct node  *node_walk(const char *path, struct node *parent,
	const struct config *cfg)
{
	struct node     *node;
	struct node     *newnode;
	struct dirent   *ent;
	struct stat     st;
	DIR             *dir;

	if (node_init(node = malloc(sizeof(struct node)), parent) != EXIT_SUCCESS)
		return (NULL);
	dir = opendir(path);
	if (!dir)
	{
		free(node);
		return NULL;
	}
	while ((ent = readdir(dir)) != NULL)
	{
		if (ent->d_name[0] == '.')
			continue ;
		ft_snprintf(node->path, PATH_MAX, "%s/%s", path, ent->d_name);
		if (ent->d_type & DT_DIR)
		{
			newnode = node_walk(node->path, node, cfg);
			if (!newnode)
				continue ;
			ft_lstpush_sort(&node->childs, ft_lstnewlink(newnode, 0),
				(cfg->flags & FLAG_REVERSE) ? lst_revcmp : lst_cmp);
		}
		else if (stat(node->path, &st) >= 0)
		{
			node->space += (size_t)st.st_size;
			node->files += 1;
		}
	}
	closedir(dir);
	node->total_space += node->space;
	node->total_files += node->files;
	node_update_parent(node);
	node->path_len = ft_strlen(path);
	ft_strcpy(node->path, path);
	return node;
}

static void show_human(struct s_printf *pf)
{
	size_t              len;
	char                buf[80];

	len = ft_printf_append(pf, buf,
		(size_t)ft_wsize((size_t)pf->raw_value, buf, 80));
	pf->slen += len;
}

static size_t strcmplen(const char *sa, const char *sb)
{
	size_t		len;

	for (len = 0; sa[len] && sa[len] == sb[len]; len++)
		;
	return (len);
}

static void node_iter_show(size_t level, struct node *node, void *config)
{
	struct config	*cfg = config;
	char			path[PATH_MAX];

	(void)level;
	ft_strcpy(path, node->path);
	if ((!(cfg->flags & FLAG_FULLPATH_DISPLAY)) && (node->parent))
		ft_memset(path, ' ', strcmplen(node->path, node->parent->path));
	ft_printf("%-*s : %-8.2lk : %-6lu\n", cfg->path_len_align,
		path, show_human,
		(cfg->flags & FLAG_LOCALSTAT) ? node->space : node->total_space,
		(cfg->flags & FLAG_LOCALSTAT) ? node->files : node->total_files);
}

/*
** get the max path lenght
** this is need for alignment purposes
*/

static void	node_iter_get_maxpl(size_t level, struct node *node, void *config)
{
	struct config	*cfg = config;

	(void)level;
	if (node->path_len > cfg->path_len_align)
		cfg->path_len_align = node->path_len;
}

#define PREFIX 1
#define SUFFIX 2

static void	node_iter(const size_t mode,
	struct node *node,
	void *userdata,
	size_t level,
	void (*f)(size_t, struct node *, void *))
{
	struct s_list	*lst;

	if (mode & PREFIX) 
		f(level, node, userdata);
	for (lst = node->childs; lst; lst = lst->next)
		node_iter(mode, lst->content, userdata, level + 1, f);
	if (mode & SUFFIX)
		f(level, node, userdata);
}

static void	node_iter_clean(size_t level, struct node *node, void *unused)
{
	(void)level;
	(void)unused;
	if (node->childs)
		ft_lstdel(&node->childs, NULL);
	free(node);
}

static int		parser(int ac, char **av, struct config *cfg)
{
	int		idx;

	ft_bzero(cfg, sizeof(*cfg));
	if (ac < 2)
	{
		ft_printf("usage: %s <path>\n", av[0]);
		return (EXIT_FAILURE);
	}
	for (idx = 1; idx < ac; idx++)
	{
		if (av[idx][0] != '-')
			cfg->root = av[idx];
		else if (!ft_strcmp(av[idx], "-p"))
			cfg->flags |= FLAG_FULLPATH_DISPLAY;
		else if (!ft_strcmp(av[idx], "-l"))
			cfg->flags |= FLAG_LOCALSTAT;
		else if (!ft_strcmp(av[idx], "-r"))
			cfg->flags |= FLAG_REVERSE;
	}
	cfg->path_len_align = 42;
	return (EXIT_SUCCESS);
}

int     main(int ac, char **av)
{
	struct config	cfg;
	struct node     *node;
	size_t			dorder = PREFIX;

	if (parser(ac, av, &cfg) != EXIT_SUCCESS)
		return (EXIT_FAILURE);
	node = node_walk(cfg.root, NULL, &cfg);
	if (cfg.flags & FLAG_REVERSE)
		dorder = SUFFIX;
	if (!node)
	{
		ft_dprintf(STDERR_FILENO, "%s%s\n", "Error: ", strerror(errno));
		return (EXIT_FAILURE);
	}
	node_iter(PREFIX, node, &cfg, 0, node_iter_get_maxpl);
	node_iter(dorder, node, &cfg, 0, node_iter_show);
	node_iter(SUFFIX, node, NULL, 0, node_iter_clean);
	return (EXIT_SUCCESS);
}
