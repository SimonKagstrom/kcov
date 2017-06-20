#pragma once

#include <stdint.h>
#include <string>
#include <sys/mman.h>

#include <utils.hh>

const uint32_t DYNINST_MAGIC = 0x4d455247; // "MERG"
const uint32_t DYNINST_VERSION = 1;

namespace kcov_dyninst
{
	struct dyninst_file
	{
		uint32_t magic;
		uint32_t version;
		uint32_t n_entries;
		uint32_t header_checksum;
		uint32_t filename_offset;
		uint32_t kcov_options_offset; // --include-pattern etc
		uint32_t data[];
	};

	class dyninst_memory
	{
	public:
		dyninst_memory(const std::string &fn, const std::string &opts, uint32_t n) :
			filename(fn),
			options(opts),
			n_entries(n)
		{
			mapped = true;
			data = (uint32_t *)::mmap(NULL, n_entries * sizeof(uint32_t), PROT_READ | PROT_WRITE,
					MAP_SHARED | MAP_ANONYMOUS, -1, 0);
			if (!data)
			{
				// Fallback to plain malloc - won't work with forks
				data = (uint32_t *)xmalloc(n_entries * sizeof(uint32_t));
			}
		}

		~dyninst_memory()
		{
			if (mapped)
			{
				munmap(data, n_entries * sizeof(uint32_t));
			}
			else
			{
				free(data);
			}
		}

		void reportIndex(uint32_t index);

		bool indexIsHit(uint32_t index);

		const std::string filename;
		const std::string options;
		const uint32_t n_entries;
		uint32_t *data;

	private:
		bool mapped;
	};

	struct dyninst_file *memoryToFile(const class dyninst_memory &mem, size_t &outSize);

	class dyninst_memory *fileToMemory(const struct dyninst_file &file);

	class dyninst_memory *diskToMemory(const std::string &src);
};
