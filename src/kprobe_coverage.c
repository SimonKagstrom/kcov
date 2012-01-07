/*
 * Copyright (C) 2010 Simon Kagstrom
 *
 * See COPYING for license details
 */
#define _GNU_SOURCE

#include <kc.h>
#include <stdio.h>
#include <utils.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

static char debugfs_path[PATH_MAX];
static const char *kprobe_coverage_write_path;
static const char *kprobe_coverage_read_path;

/* From lttng */
static int lookup_debugfs(void)
{
	int ret = -ENOENT;

	FILE *fp = fopen("/proc/mounts", "r");
	if (!fp)
		return -EINVAL;

	while (1) {
		char *lineptr = NULL;
		size_t n;

		if (getline(&lineptr, &n, fp) < 0)
			break;

		if (strncmp(lineptr, "debugfs", strlen("debugfs")) == 0) {
			char *before;
			char *after;

			before = strchr(lineptr, ' ');
			if (!before)
				panic("Wrong mounts format");
			before++;

			after = strchr(before, ' ');
			if (!after)
				panic("wrong mounts format");

			memcpy(debugfs_path, before, after - before);
		}

		free(lineptr);
	}

	kprobe_coverage_write_path = dir_concat(debugfs_path, "kprobe-coverage/control");
	kprobe_coverage_read_path = dir_concat(debugfs_path, "kprobe-coverage/show");
	panic_if (!file_exists(kprobe_coverage_write_path),
			"kprobe-coverage/control not found in debugfs\n"
			"please load the kprobe_coverage module.\n");

	fclose(fp);

	return ret;
}

static int setup_breakpoints(struct kc *kc)
{
	GHashTableIter iter;
	unsigned long key;
	struct kc_addr *val;
	FILE *fp = fopen(kprobe_coverage_write_path, "w");

	panic_if(!fp, "Can't open kprobe coverage file %s for writing\n",
			kprobe_coverage_write_path);

	g_hash_table_iter_init(&iter, kc->addrs);
	while (g_hash_table_iter_next(&iter, (gpointer*)&key, (gpointer*)&val))
		fprintf(fp, "%s%s0x%lx\n",
				kc->module_name, strlen(kc->module_name) == 0 ? "" : ":",
				val->addr);

	fclose(fp);

	return 0;
}


void kprobe_coverage_run(struct kc *kc, const char *write_path,
		const char *read_path)
{
	FILE *fp;

	/* First lookup the path */
	if (write_path && read_path) {
		kprobe_coverage_write_path = xstrdup(write_path);
		kprobe_coverage_read_path = xstrdup(read_path);
	} else
		lookup_debugfs();
	setup_breakpoints(kc);

	fp = fopen(kprobe_coverage_read_path, "r");
	panic_if (!fp, "Can't open %s for reading\n",
			kprobe_coverage_read_path);

	while (!feof(fp)) {
		char *line = NULL;
		struct kc_addr *addr;
		unsigned long l_addr;
		char *endp;
		char *p;
		size_t sz = 0;

		if (getline(&line, &sz, fp) < 0)
			break;
		p = line;
		if (strchr(p, ':'))
			p = strchr(p, ':') + 1;
		l_addr = strtoul(p, &endp, 16);
		free(line);

		if (endp == p)
			continue;
		addr = kc_lookup_addr(kc, l_addr);
		if (addr)
			kc_addr_register_hit(addr);
	}
}
