#include <sys/types.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <utils.hh>
#include <swap-endian.hh>

#include "dyninst-file-format.hh"

struct Instance
{
	uint32_t id;
	time_t last_time;

	bool initialized;
	kcov_dyninst::dyninst_memory *data;
	std::string destination_dir;
};

static Instance g_instance;
static uint32_t early_hits[4096];
static uint32_t early_hits_index;


static void write_report(unsigned int idx)
{
	size_t size;
	struct kcov_dyninst::dyninst_file *dst = kcov_dyninst::memoryToFile(*g_instance.data, size);

	if (!dst)
	{
		fprintf(stderr, "kcov-binary-lib: Can't marshal data\n");
		return;
	}

	(void)mkdir(g_instance.destination_dir.c_str(), 0755);

	std::string out = fmt("%s/%08lx", g_instance.destination_dir.c_str(), (long)g_instance.id);
	std::string tmp = fmt("%s.%u", out.c_str(), idx);
	FILE *fp = fopen(tmp.c_str(), "w");

	if (fp)
	{
		fwrite(dst, 1, size, fp);
		fclose(fp);
		rename(tmp.c_str(), out.c_str());
	}
	else
	{
		fprintf(stderr, "kcov-binary-lib: Can't write outfile\n");
	}

	free(dst);
}

static void write_at_exit(void)
{
	write_report(0);
}

static kcov_dyninst::dyninst_memory *read_report(size_t expectedSize)
{
	std::string in = fmt("%s/%08lx", g_instance.destination_dir.c_str(), (long)g_instance.id);

	if (!file_exists(in))
	{
		return NULL;
	}

	size_t sz;
	struct kcov_dyninst::dyninst_file *src = (struct kcov_dyninst::dyninst_file *)read_file(&sz, "%s", in.c_str());
	if (!src)
	{
		printf("Can't read\n");
		return NULL;
	}
	// Wrong size
	if (sz != expectedSize)
	{
		printf("Wrong size??? %zu vs %zu\n", sz, expectedSize);
		free(src);
		return NULL;
	}

	kcov_dyninst::dyninst_memory *out = kcov_dyninst::fileToMemory(*src);
	free(src);

	return out;
}

extern "C" void kcov_dyninst_binary_init(uint32_t id, size_t vectorSize, const char *filename, const char *kcovOptions)
{
	const char *path = getenv("KCOV_SYSTEM_DESTINATION_DIR");

	g_instance.id = id;
	g_instance.last_time = time(NULL);
	g_instance.destination_dir = "/tmp/kcov-data/";

	if (path)
		g_instance.destination_dir = path;

	size_t size = (vectorSize + 31) / 32;

	g_instance.data = read_report(strlen(filename) + strlen(kcovOptions) + 2 + size * sizeof(uint32_t) +
			sizeof(struct kcov_dyninst::dyninst_file));
	if (!g_instance.data)
	{
		g_instance.data = new kcov_dyninst::dyninst_memory(filename, kcovOptions, size);
	}

	atexit(write_at_exit);
	g_instance.initialized = true;
}

extern "C" void kcov_dyninst_binary_report_address(uint32_t bitIdx)
{
	// Handle hits which happen before we're initialized
	if (!g_instance.initialized)
	{
		uint32_t dst = __sync_fetch_and_add(&early_hits_index, 1);

		if (dst >= sizeof(early_hits) / sizeof(early_hits[0]))
		{
			fprintf(stderr, "kcov: Library not initialized yet and hit table full, missing point %u\n", bitIdx);
			return;
		}
		early_hits[dst] = bitIdx;
		return;
	}

	if (early_hits_index != 0)
	{
		uint32_t to_loop = early_hits_index;

		early_hits_index = 0; // Avoid inifite recursion
		for (uint32_t i = 0; i < to_loop; i++)
		{
			kcov_dyninst_binary_report_address(early_hits[i]);
		}
	}
	g_instance.data->reportIndex(bitIdx);

	// Write out the report
	time_t now = time(NULL);
	if (now - g_instance.last_time >= 2)
	{
		write_report(bitIdx);
		g_instance.last_time = now;
	}
}
