#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "../kernel/include/linux/prinfo.h"


void print_process(const struct prinfo pr, int indent) {
	int i;

	for (i = 0; i < indent; i ++)
		printf("\t");
	printf("%s,%d,%ld,%d,%d,%d,%ld\n", pr.comm, pr.pid, pr.state,
		pr.parent_pid, (int)pr.first_child_pid, pr.next_sibling_pid, pr.uid);
}


int dfs_print(struct prinfo *tree, const int size, int cur, int indent) {
	int cur_pid;

	if (cur >= size)
		return cur;

	print_process(tree[cur], indent);

	cur_pid = tree[cur++].pid;
	while (cur < size && tree[cur].parent_pid == cur_pid) {
		cur = dfs_print(tree, size, cur, indent + 1);
	}

	return cur;
}


int main()
{
	struct prinfo *buf = NULL;
	int nr = 50;
	int ret;
	
	buf = (struct prinfo *)malloc(nr * sizeof(struct prinfo));
	ret = syscall(245, NULL, &nr);

	if (ret < 0) {
		fprintf(stderr, "Error: %d\n", ret);
		return 0;
	}
	
	/*
	printf("\n===============\nTotal entries: %d\n===============\n", ret);
	printf("\n===============\nNr: %d\n===============\n", nr);
	*/
	
	dfs_print(buf, nr, 0, 0);
	
	return 0;
}

