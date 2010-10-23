#ifndef __KC_H__
#define __KC_H__

#include "kc_elf.h"
#include "kc_file.h"
#include "kc_line.h"
#include "kc_addr.h"

#include <stddef.h>
#include <glib.h>
#include <pthread.h>

enum
{
	KPROBE_COVERAGE,
	PTRACE_FILE,
	PTRACE_PID,
};

enum report_sort_type
{
	FILENAME,
	COVERAGE_PERCENT,
};

struct kc
{
	const char *out_dir;
	const char *in_file;

	const char **only_report_paths;
	const char **exclude_paths;
	const char *module_name;
	enum report_sort_type sort_type;
	int type;

	ptrdiff_t base_addr;

	GHashTable *addrs;
	GHashTable *files;
	GHashTable *lines;

	GList *hit_list;
	pthread_mutex_t pending_mutex;
	GList *pending_list;
};

struct kc_debug_backend
{

};

extern struct kc *kc_open_elf(const char *filename);

extern void kc_close(struct kc *kc);

extern struct kc_addr *kc_lookup_addr(struct kc *kc, unsigned long addr);

extern void run_report_thread(const char *dir, struct kc *kc);

extern void stop_report_thread(void);


extern void kc_add_addr(struct kc *kc, unsigned long addr, int line, const char *filename);

extern void kc_read_back_breakpoints(struct kc *kc, const char *filename);

extern void kc_generate_webpage(struct kc *kc, const char *outdir);


/* Kernel */
extern void kprobe_coverage_run(struct kc *kc,
		const char *write_file, const char *read_file);

/* Ptrace */
extern int ptrace_run(struct kc *kc, char *const argv[]);

#endif
