#include "dyninst-file-format.hh"

#include <utils.hh>

struct kcov_dyninst::dyninst_file *kcov_dyninst::memoryToFile(const class kcov_dyninst::dyninst_memory &mem, size_t &outSize)
{
	outSize = sizeof(struct dyninst_file) + mem.n_entries * sizeof(uint32_t) +
			mem.filename.size() + mem.options.size() + 2;
	struct dyninst_file *out = (struct dyninst_file *)xmalloc(outSize);
	char *p = (char *)out;

	out->magic = DYNINST_MAGIC;
	out->version = DYNINST_VERSION;
	out->n_entries = mem.n_entries;
	out->filename_offset = mem.n_entries * sizeof(uint32_t) + sizeof(*out);
	out->kcov_options_offset = out->filename_offset + mem.filename.size() + 1;

	strcpy(&p[out->filename_offset], mem.filename.c_str());
	strcpy(&p[out->kcov_options_offset], mem.options.c_str());

	memcpy(out->data, mem.data, out->n_entries * sizeof(uint32_t));

	return out;
}

class kcov_dyninst::dyninst_memory *kcov_dyninst::fileToMemory(const struct kcov_dyninst::dyninst_file &file)
{
	if (file.magic != DYNINST_MAGIC)
	{
		return NULL;
	}

	if (file.version != DYNINST_VERSION)
	{
		return NULL;
	}

	size_t sizeExceptStrings = sizeof(file) + file.n_entries * sizeof(uint32_t);

	if (file.filename_offset < sizeExceptStrings)
	{
		return NULL;
	}

	if (file.kcov_options_offset < file.filename_offset + 1)
	{
		return NULL;
	}
	char *p = (char *)&file;

	kcov_dyninst::dyninst_memory *out = new kcov_dyninst::dyninst_memory(std::string(&p[file.filename_offset]),
			std::string(&p[file.kcov_options_offset]),
			file.n_entries);

	memcpy(out->data, file.data, out->n_entries * sizeof(uint32_t));

	return out;
}

class kcov_dyninst::dyninst_memory *kcov_dyninst::diskToMemory(const std::string &src)
{
	size_t sz;
	kcov_dyninst::dyninst_file *p = (kcov_dyninst::dyninst_file *)read_file(&sz, "%s", src.c_str());

	if (!p)
	{
		return NULL;
	}

	kcov_dyninst::dyninst_memory *out = kcov_dyninst::fileToMemory(*p);
	free(p);

	return out;
}

bool kcov_dyninst::dyninst_memory::indexIsHit(uint32_t index)
{
	if (index / 32 >= n_entries)
	{
		return false;
	}

	uint32_t cur = data[index / 32];
	uint32_t bit = index % 32;

	return !!(cur & (1 << bit));
}

void kcov_dyninst::dyninst_memory::reportIndex(uint32_t index)
{
	unsigned int wordIdx = index / 32;
	unsigned int bit = index % 32;

	if (wordIdx >= n_entries)
	{
		return;
	}

	// Update the bit atomically
	uint32_t *p = &data[wordIdx];

	// Already hit?
	if (*p & (1 << bit))
		return;

	uint32_t val, newVal;
	do
	{
		val = *p;
		newVal = val | (1 << bit);

	} while (!__sync_bool_compare_and_swap(p, val, newVal));
}
