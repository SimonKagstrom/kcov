/*
 * Copyright (C) 2010 Simon Kagstrom, Thomas Neumann
 *
 * See COPYING for license details
 */
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <libgen.h>
#include <getopt.h>

#include <kc.h>
#include <utils.h>

#include <kc_ptrace_arch.h>

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
static const char **exclude_paths = NULL;
static const char **only_report_patterns = NULL;
static const char **exclude_patterns = NULL;
static char *const *program_args;
static const char *title = NULL;
static unsigned long low_limit = 16;
static unsigned long high_limit = 50;
static unsigned long path_strip_level = 2;
static pid_t ptrace_pid;

static void usage(void)
{
	printf("Usage: kcov [OPTIONS] out-dir in-file [args...]\n"
	       "\n"
	       "Where [OPTIONS] are\n"
	       "  -h, --help             this text\n"
	       "  -p, --pid=PID          trace PID instead of executing in-file,\n"
	       "                         in-file is optional in this case\n"
	       "  -s, --sort-type=type   how to sort files: f[ilename] (default), p[ercent]\n"
	       "  -l, --limits=low,high  setup limits for low/high coverage (default %lu,%lu)\n"
	       "  -t, --title=title      title for the coverage (default: filename)\n"
	       "  --path-strip-level=num number of path levels to show for common paths (default: %lu)\n"
	       "  --include-path=path    comma-separated list of paths to include in the report\n"
	       "  --exclude-path=path    comma-separated list of paths to exclude in the report\n"
	       "  --include-pattern=pat  comma-separated path patterns to include in the report\n"
	       "  --exclude-pattern=pat  comma-separated path patterns to exclude in the report\n"
	       "\n"
	       "Examples:\n"
	       "  kcov /tmp/frodo ./frodo          # Check coverage for ./frodo\n"
	       "  kcov --pid=1000 /tmp/frodo       # Check coverage for PID 1000\n"
	       "  kcov --include-pattern=/src/frodo/ /tmp/frodo ./frodo  # Only include files\n"
	       "                                                         # including /src/frodo\n"
	       "",
	       low_limit, high_limit, path_strip_level);
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

static const char **get_comma_separated_pathvec(const char *src)
{
	char *cpy = xstrdup(src);
	char **pathvec = NULL;
	char *path;
	int n = 0;

	path = strtok(cpy, ",");
	do
	{
		const char *p = expand_path(path);

		pathvec = xrealloc(pathvec, sizeof(const char *) * (n + 2));
		pathvec[n] = realpath(p, NULL);
		if (pathvec[n] == NULL) {
			fprintf(stderr, "Can't resolve path %s\n", p);
			usage();
		}

		pathvec[n + 1] = NULL;
		n++;
		path = strtok(NULL, ",");
		free((void *)p);
	} while(path);

	free(cpy);

	return (const char **)pathvec;
}

static void parse_arguments(int argc, char *const argv[])
{
	static const struct option long_options[] = {
			{"help", no_argument, 0, 'h'},
			{"pid", required_argument, 0, 'p'},
			{"sort-type", required_argument, 0, 's'},
			{"limits", required_argument, 0, 'l'},
			{"title", required_argument, 0, 't'},
			{"path-strip-level", required_argument, 0, 'S'},
			{"exclude-pattern", required_argument, 0, 'x'},
			{"include-pattern", required_argument, 0, 'i'},
			{"exclude-path", required_argument, 0, 'X'},
			{"include-path", required_argument, 0, 'I'},
			/*{"write-file", required_argument, 0, 'w'}, Take back when the kernel stuff works */
			/*{"read-file", required_argument, 0, 'r'}, Ditto */
			{0,0,0,0}
	};
	int after_opts = 0;
	int extra_needed = 2;
	int last_arg;

	/* Scan through the parameters for an ELF file: That will be the
	 * second last argument in the list.
	 *
	 * After that it's arguments to the external program.
	 */
	for (last_arg = 1; last_arg < argc; last_arg++) {
		if (kc_is_elf(argv[last_arg]))
			break;
	}

	while (1) {
		char *endp;
		int option_index = 0;
		int c;

		c = getopt_long (last_arg, argv, "hp:s:l:t:",
				long_options, &option_index);

		/* No more options */
		if (c == -1)
			break;

		switch (c) {
		case 0:
			break;
		case 'h':
			usage();
			break;
		case 'p':
			ptrace_pid = strtoul(optarg, &endp, 0);
			extra_needed = 1;
			break;
		case 's':
			sort_type = optarg;
			break;
		case 't':
			title = optarg;
			break;
		case 'i':
			only_report_patterns = get_comma_separated_strvec(optarg);
			break;
		case 'S':
			path_strip_level = strtoul(optarg, &endp, 0);
			if (endp == optarg || *endp != '\0')
				usage();
			if (path_strip_level == 0)
				path_strip_level = INT_MAX;
			break;
		case 'x':
			exclude_patterns = get_comma_separated_strvec(optarg);
			break;
		case 'I':
			only_report_paths = get_comma_separated_pathvec(optarg);
			break;
		case 'X':
			exclude_paths = get_comma_separated_pathvec(optarg);
			break;
		case 'l': {
			const char **limits = get_comma_separated_strvec(optarg);

			if (limits[0] == NULL || limits[1] == NULL)
				usage();

			low_limit = strtoul(limits[0], &endp, 0);
			if (endp == limits[0])
				usage();
			high_limit = strtoul(limits[1], &endp, 0);
			if (endp == limits[1])
				usage();

			while (*limits)
			{
				free((void *)*limits);
				limits++;
			}
			break;
		}
		case 'r':
			read_path = optarg;
			break;
		case 'w':
			write_path = optarg;
			break;
		default:
			error("Unrecognized option: -%c\n", optopt);
		}
	}

	after_opts = optind;

	/* When tracing by PID, the filename is optional */
	if (argc < after_opts + extra_needed)
		usage();

	out_dir = argv[after_opts];
	if (argc >= after_opts + 2)
		in_file = argv[after_opts + 1];
	program_args = &argv[after_opts + 1];
}

int main(int argc, char *argv[])
{
	struct kc *kc;
	char *bn;

	if (argc < 2)
		usage();

	kc_ptrace_arch_setup();

	parse_arguments(argc, argv);

	kc = kc_open_elf(in_file, ptrace_pid);
	if (!kc)
		usage();
	bn = strdup(in_file);

	kc->out_dir = out_dir;
	kc->binary_filename = basename(bn);
	kc->only_report_paths = only_report_paths;
	kc->exclude_paths = exclude_paths;
	kc->only_report_patterns = only_report_patterns;
	kc->exclude_patterns = exclude_patterns;
	kc->low_limit = low_limit;
	kc->high_limit = high_limit;
	kc->path_strip_level = path_strip_level;

	panic_if(!kc->binary_filename, "basename failed\n");

	/* Re-read the old settings, if it exists */
	kc_read_db(kc);

	if (sort_type) {
		if (sort_type[0] == 'f')
			kc->sort_type = FILENAME;
		else if (sort_type[0] == 'p')
			kc->sort_type = COVERAGE_PERCENT;
	}

	run_report_thread(kc);

	signal(SIGINT, on_ctrlc);

	switch(kc->type) {
	case PTRACE_FILE:
		if (title)
			kc->module_name = xstrdup(title);
		else
			kc->module_name = xstrdup(kc->in_file);
		ptrace_run(kc, program_args);
		break;
	case PTRACE_PID:
		kc->module_name = xstrdup(kc->in_file);
		ptrace_pid_run(kc, ptrace_pid);
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
