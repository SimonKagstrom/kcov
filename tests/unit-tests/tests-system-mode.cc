#include "test.hh"

#include <configuration.hh>
#include <utils.hh>

#include "../../src/engines/system-mode-file-format.hh"

#include <system-mode/file-data.hh>
#include <system-mode/registration.hh>

using namespace kcov_system_mode;
using namespace kcov;

TESTSUITE(system_mode_formats)
{
	TEST(can_marshal_empty_struct)
	{
		class system_mode_memory mem("kalle-anka", "--include-pattern=anka --exclude-pattern=hejbo", 0);

		size_t size;
		auto file = memoryToFile(mem, size);
		char *p = (char *)file;
		ASSERT_TRUE(file);

		ASSERT_TRUE(file->magic == SYSTEM_MODE_FILE_MAGIC);
		ASSERT_TRUE(file->version == SYSTEM_MODE_FILE_VERSION);
		ASSERT_TRUE(file->n_entries == 0);
		ASSERT_TRUE(file->filename_offset == sizeof(*file));
		ASSERT_TRUE(file->kcov_options_offset == file->filename_offset + mem.filename.size() + 1);

		auto fn = std::string(&p[file->filename_offset]);
		auto opts = std::string(&p[file->kcov_options_offset]);

		ASSERT_TRUE(fn == "kalle-anka");
		ASSERT_TRUE(opts == "--include-pattern=anka --exclude-pattern=hejbo");
	}

	TEST(can_marshal_single_entry_struct)
	{
		class system_mode_memory mem("roy-gunnar-ramstedt", "moa", 1);

		mem.data[0] = 0x12345678;

		size_t size;
		auto file = memoryToFile(mem, size);
		char *p = (char *)file;
		ASSERT_TRUE(file);

		ASSERT_TRUE(file->n_entries == 1);
		ASSERT_TRUE(file->data[0] == 0x12345678);

		auto fn = std::string(&p[file->filename_offset]);
		auto opts = std::string(&p[file->kcov_options_offset]);

		ASSERT_TRUE(fn == "roy-gunnar-ramstedt");
		ASSERT_TRUE(opts == "moa");
	}

	TEST(can_marshal_multi_entry_struct)
	{
		class system_mode_memory mem("", "simon", 3);

		mem.data[0] = 0x12345678;
		mem.data[1] = 0x56789abc;
		mem.data[2] = 0xaabbccdd;

		size_t size;
		auto file = memoryToFile(mem, size);
		char *p = (char *)file;
		ASSERT_TRUE(file);

		ASSERT_TRUE(file->n_entries == 3);
		ASSERT_TRUE(file->data[0] == 0x12345678);
		ASSERT_TRUE(file->data[1] == 0x56789abc);
		ASSERT_TRUE(file->data[2] == 0xaabbccdd);

		auto fn = std::string(&p[file->filename_offset]);
		auto opts = std::string(&p[file->kcov_options_offset]);

		ASSERT_TRUE(fn == "");
		ASSERT_TRUE(opts == "simon");
	}

	TEST(cannot_unmarshal_with_invalid_magic)
	{
		struct system_mode_file f{};

		f.magic = SYSTEM_MODE_FILE_MAGIC+1;
		f.version = SYSTEM_MODE_FILE_VERSION;

		auto mem = fileToMemory(f);
		ASSERT_FALSE(mem);
	}

	TEST(cannot_unmarshal_with_invalid_version)
	{
		struct system_mode_file f{};

		f.magic = SYSTEM_MODE_FILE_MAGIC;
		f.version = SYSTEM_MODE_FILE_VERSION + 1;

		auto mem = fileToMemory(f);
		ASSERT_FALSE(mem);
	}

	TEST(cannot_unmarshal_with_invalid_string_offsets)
	{
		struct system_mode_file f{};

		f.magic = SYSTEM_MODE_FILE_MAGIC;
		f.version = SYSTEM_MODE_FILE_VERSION;

		f.filename_offset = 0; // Should be atleast sizeof(f)

		auto mem = fileToMemory(f);
		ASSERT_FALSE(mem);

		f.filename_offset = sizeof(f);
		f.kcov_options_offset = sizeof(f); // Should be at least + 1
		mem = fileToMemory(f);

		ASSERT_FALSE(mem);
	}

	TEST(can_unmarshal_empty_struct)
	{
		struct system_mode_file *f = (struct system_mode_file *)xmalloc(sizeof(struct system_mode_file) + 2);

		f->magic = SYSTEM_MODE_FILE_MAGIC;
		f->version = SYSTEM_MODE_FILE_VERSION;
		f->filename_offset = sizeof(*f);
		f->kcov_options_offset = f->filename_offset + 1;

		auto mem = fileToMemory(*f);
		ASSERT_TRUE(mem);

		ASSERT_TRUE(mem->n_entries == 0);
		ASSERT_TRUE(mem->filename == "");
		ASSERT_TRUE(mem->options == "");
	}

	TEST(can_unmarshal_single_entry_struct)
	{
		struct system_mode_file *f = (struct system_mode_file *)xmalloc(sizeof(struct system_mode_file) +
				sizeof(uint32_t) +
				strlen("hej") + strlen("hopp") + 2);
		char *p = (char *)f;

		f->magic = SYSTEM_MODE_FILE_MAGIC;
		f->version = SYSTEM_MODE_FILE_VERSION;
		f->n_entries = 1;
		f->filename_offset = sizeof(*f) + sizeof(uint32_t);
		f->kcov_options_offset = f->filename_offset + strlen("hej") + 1;
		strcpy(&p[f->filename_offset], "hej");
		strcpy(&p[f->kcov_options_offset], "hopp");
		f->data[0] = 0x12345678;


		auto mem = fileToMemory(*f);
		ASSERT_TRUE(mem);

		ASSERT_TRUE(mem->filename == "hej");
		ASSERT_TRUE(mem->options == "hopp");
		ASSERT_TRUE(mem->n_entries == 1);
		ASSERT_TRUE(mem->data[0] == 0x12345678);
	}

	TEST(can_unmarshal_multi_entry_struct)
	{
		struct system_mode_file *f = (struct system_mode_file *)xmalloc(sizeof(struct system_mode_file) +
				sizeof(uint32_t) * 3 +
				strlen("a") + strlen("b") + 2);
		char *p = (char *)f;

		f->magic = SYSTEM_MODE_FILE_MAGIC;
		f->version = SYSTEM_MODE_FILE_VERSION;
		f->n_entries = 3;
		f->filename_offset = sizeof(*f) + sizeof(uint32_t) * 3;
		f->kcov_options_offset = f->filename_offset + strlen("hej") + 1;
		strcpy(&p[f->filename_offset], "a");
		strcpy(&p[f->kcov_options_offset], "b");
		f->data[0] = 0x12345678;
		f->data[1] = 123;
		f->data[2] = 4711;


		auto mem = fileToMemory(*f);
		ASSERT_TRUE(mem);

		ASSERT_TRUE(mem->filename == "a");
		ASSERT_TRUE(mem->options == "b");
		ASSERT_TRUE(mem->n_entries == 3);
		ASSERT_TRUE(mem->data[0] == 0x12345678);
		ASSERT_TRUE(mem->data[1] == 123);
		ASSERT_TRUE(mem->data[2] == 4711);
	}

	TEST(out_of_bounds_indexes_are_not_hit)
	{
		class system_mode_memory mem("roy-gunnar-ramstedt", "moa", 1);
		class system_mode_memory memEmpty("roy-gunnar-ramstedt", "moa", 0);

		// 0..31 are possible
		ASSERT_FALSE(mem.indexIsHit(32));
		ASSERT_FALSE(memEmpty.indexIsHit(0));
	}

	TEST(can_report_single_hit)
	{
		class system_mode_memory mem("roy-gunnar-ramstedt", "moa", 1);

		mem.reportIndex(1);

		ASSERT_FALSE(mem.indexIsHit(0));
		ASSERT_TRUE(mem.indexIsHit(1));
	}

	TEST(can_report_multiple_hits)
	{
		class system_mode_memory mem("roy-gunnar-ramstedt", "moa", 4);

		for (unsigned i = 0; i < 4 * 32; i += 2)
		{
			mem.reportIndex(i);
		}

		for (unsigned i = 0; i < 4 * 32; i++)
		{
			if (i % 2 == 0)
			{
				ASSERT_TRUE(mem.indexIsHit(i));
			}
			else
			{
				ASSERT_FALSE(mem.indexIsHit(i));
			}
		}
	}

	TEST(can_detect_dirtyness)
	{
		class system_mode_memory mem("roy-gunnar-ramstedt", "moa", 4);

		ASSERT_FALSE(mem.isDirty());

		mem.reportIndex(4 * 32 - 1);
		ASSERT_TRUE(mem.isDirty());

		mem.markClean();
		ASSERT_FALSE(mem.isDirty());

		mem.reportIndex(4 * 32 - 2);
		ASSERT_TRUE(mem.isDirty());
	}
}
