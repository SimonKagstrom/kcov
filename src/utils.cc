#include <sched.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <libelf.h>

#include "utils.hh"

int g_kcov_debug_mask;

extern int file_is_elf(const char *filename)
{
	Elf32_Ehdr hdr;
	int ret = 0;
	int fd;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return ret;

	/* Compare the header with the ELF magic */
	if (read(fd, &hdr, sizeof(hdr)) == sizeof(hdr))
		ret = memcmp(hdr.e_ident, ELFMAG, strlen(ELFMAG)) == 0;

	close(fd);

	return ret;
}

void *read_file(size_t *out_size, const char *fmt, ...)
{
	struct stat buf;
	char path[2048];
	va_list ap;
	void *data;
	size_t size;
	FILE *f;
	int r;

	/* Create the filename */
	va_start(ap, fmt);
	r = vsnprintf(path, 2048, fmt, ap);
	va_end(ap);

	panic_if (r >= 2048,
			"Too long string!");

	if (lstat(path, &buf) < 0)
		return NULL;

	size = buf.st_size;
	data = xmalloc(size + 2); /* NULL-terminate, if used as string */
	f = fopen(path, "r");
	if (!f)
	{
		free(data);
		return NULL;
	}
	if (fread(data, 1, size, f) != size)
	{
		free(data);
		data = NULL;
	}
	fclose(f);

	*out_size = size;

	return data;
}

int write_file(const void *data, size_t len, const char *fmt, ...)
{
	char path[2048];
	va_list ap;
	FILE *fp;
	int ret = 0;

	/* Create the filename */
	va_start(ap, fmt);
	vsnprintf(path, 2048, fmt, ap);
	va_end(ap);

	fp = fopen(path, "w");
	if (!fp)
		return -1;

	if (fwrite(data, sizeof(uint8_t), len, fp) != len)
		ret = -1;
	fclose(fp);

	return ret;
}

const char *dir_concat(const char *dir, const char *filename)
{
	size_t len;
	char *out;

	if (dir == NULL)
		return xstrdup(filename);

	len = strlen(dir) + strlen(filename) + 4;
	out = (char *)xmalloc(len);

	xsnprintf(out, len, "%s/%s", dir, filename);

	return out;
}

const char *expand_path(const char *path)
{
	size_t p_len = strlen(path);
	const char *home = getenv("HOME");
	size_t home_len;
	char *out;

	if (!home || p_len < 1 || path[0] != '~')
		return xstrdup(path);

	home_len = strlen(home);
	out = (char *)xmalloc(home_len + p_len + 8);
	snprintf(out, home_len + p_len + 8, "%s/%s", home, path + 1);

	return out;
}


int file_exists(const char *path)
{
	struct stat st;

	return lstat(path, &st) == 0;
}

static void read_write(FILE *dst, FILE *src)
{
	char buf[1024];

	while (!feof(src)) {
		int n = fread(buf, sizeof(char), sizeof(buf), src);

		fwrite(buf, sizeof(char), n, dst);
	}
}

int concat_files(const char *dst_name, const char *file_a, const char *file_b)
{
	FILE *dst;
	FILE *s1, *s2;
	int ret = -1;

	dst = fopen(dst_name, "w");
	if (!dst)
		return -1;
	s1 = fopen(file_a, "r");
	if (!s1)
		goto out_dst;
	s2 = fopen(file_b, "r");
	if (!s2)
		goto out_s1;

	read_write(dst, s1);
	read_write(dst, s2);

	fclose(s2);
out_s1:
	fclose(s1);
out_dst:
	fclose(dst);

	return ret;
}

unsigned long get_aligned(unsigned long addr)
{
	return (addr / sizeof(long)) * sizeof(long);
}

unsigned long get_aligned_4b(unsigned long addr)
{
	return addr & ~3;
}

std::string fmt(const char *fmt, ...)
{
	char buf[4096];
	va_list ap;
	int res;

	va_start(ap, fmt);
	res = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	panic_if(res >= (int)sizeof(buf),
			"Buffer overflow");

	return std::string(buf);
}


int kcov_get_current_cpu(void)
{
	return sched_getcpu();
}

void kcov_tie_process_to_cpu(pid_t pid, int cpu)
{
	// Switching CPU while running will cause icache
	// conflicts. So let's just forbid that.

	cpu_set_t *set = CPU_ALLOC(1);
	panic_if (!set,
			"Can't allocate CPU set!\n");
	CPU_ZERO_S(CPU_ALLOC_SIZE(1), set);
	CPU_SET(cpu, set);
	panic_if (sched_setaffinity(pid, CPU_ALLOC_SIZE(1), set) < 0,
			"Can't set CPU affinity. Coincident won't work");
	CPU_FREE(set);
}

void mdelay(unsigned int ms)
{
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = ms * 1000 * 1000;

	nanosleep(&ts, NULL);
}
