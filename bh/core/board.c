#include <board.h>

static struct board_desc *g_active_board; // or just board_id ?

static const struct board_id *match_id(const struct board_id *id, const char *name)
{
	while (id->name && id->mach_id) {
		if (!strncmp(id->name, name, sizeof(name))) // strncasecmp() instead
			return id;

		id++;
	}

	return id;
}

int __INIT__ board_init(void)
{
	int ret;
	char name[CONF_VAL_LEN];
	const struct board_id *id;
	struct board_desc *board;
	extern struct board_desc __board_start[], __board_end[];

	if (__board_start == __board_end)
		return -ENOENT;

	ret = conf_get_attr("board.id", name);
	if (ret < 0) {
		printf("ERROR: board id NOT configured!\n");
		return ret;
	}

	for (board = __board_start; board < __board_end; board++) {
		id = match_id(board->id_table, name);
		if (id) {
			ret = board->init(board, id);
			if (!ret) {
				printf("board \"%s\" is active for \"%s\"\n", id->name, name);
				g_active_board = board;
			}

			return ret;
		}
	}

	printf("No match board found for \"%s\"\n", name);

	return -ENOENT;
}

struct board_desc *board_get_active()
{
	return g_active_board;
}

void board_set_active(struct board_desc * board)
{
	g_active_board = board;
}
