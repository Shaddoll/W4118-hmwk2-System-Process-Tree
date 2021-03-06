#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "../kernel/include/linux/prinfo.h"


void print_process(const struct prinfo pr, int indent)
{
	int i;

	for (i = 0; i < indent; i++)
		printf("\t");
	printf("%s,%d,%ld,%d,%d,%d,%ld\n", pr.comm, pr.pid, pr.state,
		pr.parent_pid, pr.first_child_pid, pr.next_sibling_pid, pr.uid);
}


int dfs_print(struct prinfo *tree, const int size, int cur, int indent)
{
	int cur_pid;

	if (cur >= size)
		return cur;

	print_process(tree[cur], indent);

	cur_pid = tree[cur++].pid;
	while (cur < size && tree[cur].parent_pid == cur_pid)
		cur = dfs_print(tree, size, cur, indent + 1);

	return cur;
}


int main(void)
{
	struct prinfo *buf = NULL;
	int nr = 200;
	int ret;

	buf = (struct prinfo *)malloc(nr * sizeof(struct prinfo));

	if (buf == NULL) {
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return 0;
	}

	ret = syscall(245, buf, &nr);

	if (ret < 0) {
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return 0;
	}

	dfs_print(buf, nr, 0, 0);

	return 0;
}

