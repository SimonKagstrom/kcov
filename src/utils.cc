
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
#include <limits.h>
#include <time.h>

#include "utils.hh"

#include <sstream>
#include <stdexcept>
#include <algorithm>

int g_kcov_debug_mask = STATUS_MSG;

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

std::string dir_concat(const std::string &dir, const std::string &filename)
{
	if (dir == "")
		return filename;

	return fmt("%s/%s", dir.c_str(), filename.c_str());
}

const char *get_home(void)
{
	return getenv("HOME");
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

uint64_t get_ms_timestamp(void)
{
	return ((uint64_t)time(NULL)) * 1000;
}

bool machine_is_64bit(void)
{
	return sizeof(unsigned long) == 8;
}


// http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c
static std::list<std::string> &split(const std::string &s, char delim,
		std::list<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;

    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }

    return elems;
}


std::list<std::string> split_string(const std::string &s, const char *delims)
{
    std::list<std::string> elems;
    split(s, *delims, elems);

    return elems;
}

std::string trim_string(const std::string &strIn)
{
	std::string str = strIn;
	size_t endpos = str.find_last_not_of(" \t\n\r");

	if (std::string::npos != endpos)
		str = str.substr( 0, endpos+1 );

	// trim leading spaces
	size_t startpos = str.find_first_not_of(" \t");
	if (std::string::npos != startpos)
		str = str.substr( startpos );
	else
		str = "";

	return str;
}

std::string get_real_path(const std::string &path)
{
	char *rp = NULL;

	rp = ::realpath(path.c_str(), nullptr);

	if (!rp)
		return path;

	std::string out(rp);
	free(rp);

	return out;
}


bool string_is_integer(const std::string &str, unsigned base)
{
	size_t pos;

	try
	{
		stoull(str, &pos, base);
	}
	catch(std::invalid_argument &e)
	{
		return false;
	}
	catch(std::out_of_range &e)
	{
		return false;
	}

	return pos == str.size();
}

int64_t string_to_integer(const std::string &str, unsigned base)
{
	size_t pos;

	return (int64_t)stoull(str, &pos, base);
}
