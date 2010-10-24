#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

#include <kc.h>
#include <utils.h>

static void on_ctrlc(int sig)
{
	stop_report_thread();
	exit(0);
}

static const char *write_path = NULL;
static const char *read_path = NULL;
static const char *out_dir = NULL;
static const char *sort_type = NULL;
static const char *in_file = NULL;
static const char **only_report_paths = NULL;
static const char **exclude_paths = (const char *[]){NULL};
static char *const *program_args;
static unsigned long low_limit = 16;
static unsigned long high_limit = 50;

static void usage(void)
{
	printf("Usage: kcov [OPTIONS] out-dir in-file [args...]\n\n"
			"Where [OPTIONS] are\n"
			"  -s sort-type             how to sort files: f[ile] (filename, default), p[ercentage]\n"
			"  -l low,high              setup limits for low/high coverage (default %lu,%lu)\n"
			"  -p only-include-paths    comma-separated list of paths to include in the report\n"
			"  -x exclude-paths         comma-separated list of paths to exclude in the report\n"
			"  -w write-file            file to write breakpoints to for kernel usage\n"
			"  -r read-file             file to read hit breakpoints from for kernel usage\n"
			"", low_limit, high_limit);
	exit(1);
}

static const char **get_comma_separated_strvec(const char *src)
{
	char *cpy = xstrdup(src);
	char **strvec = NULL;
	char *p;
	int n = 0;

	p = strtok(cpy, ",");
	do
	{
		strvec = xrealloc(strvec, sizeof(const char *) * (n + 2));
		strvec[n] = xstrdup(p);
		strvec[n + 1] = NULL;
		n++;
		p = strtok(NULL, ",");
	} while (p);

	free(cpy);

	return (const char **)strvec;
}

static void parse_arguments(int argc, char *const argv[])
{
	int i;
	int after_opts = 0;

	for (i = 0; i < argc; i++) {
		const char *cur = argv[i];

		if (strcmp(cur, "-p") == 0 && i < argc - 1) {
			only_report_paths = get_comma_separated_strvec(argv[i + 1]);

			i++;
			after_opts = i + 1;
			continue;
		}
		if (strcmp(cur, "-s") == 0 && i < argc - 1) {
			sort_type = argv[i + 1];

			i++;
			after_opts = i + 1;
			continue;
		}
		if (strcmp(cur, "-l") == 0 && i < argc - 1) {
			const char **limits = get_comma_separated_strvec(argv[i + 1]);
			char *endp;
			const char **p = limits;

			if (limits[0] == NULL || limits[1] == NULL)
				usage();

			low_limit = strtoul(limits[0], &endp, 0);
			if (endp == limits[0])
				usage();
			high_limit = strtoul(limits[1], &endp, 0);
			if (endp == limits[1])
				usage();

			i++;
			after_opts = i + 1;
			while (*p)
			{
				free((void*)*p);
				p++;
			}
			continue;
		}
		if (strcmp(cur, "-x") == 0 && i < argc - 1) {
			exclude_paths = get_comma_separated_strvec(argv[i + 1]);

			i++;
			after_opts = i + 1;
			continue;
		}
		if (strcmp(cur, "-r") == 0 && i < argc - 1) {
			read_path = argv[i + 1];
			i++;
			after_opts = i + 1;
			continue;
		}
		if (strcmp(cur, "-w") == 0 && i < argc - 1) {
			write_path = argv[i + 1];
			i++;
			after_opts = i + 1;
			continue;
		}
	}

	if (argc < after_opts + 2)
		usage();
	out_dir = argv[after_opts];
	in_file = argv[after_opts + 1];
	program_args = &argv[after_opts + 1];
}

int main(int argc, char *argv[])
{
	struct kc *kc;

	if (argc < 2)
		usage();
	parse_arguments(argc - 1, &argv[1]);
	if (!only_report_paths) {
		only_report_paths = xmalloc(sizeof(const char *) * 2);
		only_report_paths[0] = "";
		only_report_paths[1] = NULL;
	}

	kc = kc_open_elf(in_file);
	if (!kc)
		usage();
	kc->out_dir = out_dir;
	kc->in_file = in_file;
	kc->only_report_paths = only_report_paths;
	kc->exclude_paths = exclude_paths;
	kc->low_limit = low_limit;
	kc->high_limit = high_limit;

	/* Re-read the old settings, if it exists */
	kc_read_db(kc);

	if (sort_type) {
		if (sort_type[0] == 'f')
			kc->sort_type = FILENAME;
		else if (sort_type[0] == 'p')
			kc->sort_type = COVERAGE_PERCENT;
	}

	run_report_thread(out_dir, kc);

	signal(SIGINT, on_ctrlc);

	switch(kc->type) {
	case PTRACE_FILE:
		kc->module_name = xstrdup(in_file);
		ptrace_run(kc, program_args);
		break;
	case KPROBE_COVERAGE:
		kprobe_coverage_run(kc, write_path, read_path);
		break;
	default:
		panic("Unsupported/unimplemented probe type %d\n", kc->type);
		break;
	}
	stop_report_thread();

	return 0;
}
