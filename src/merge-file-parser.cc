#include <file-parser.hh>
#include <collector.hh>
#include <utils.hh>
#include <filter.hh>
#include <writer.hh>
#include <lineid.hh>

#include <string>
#include <list>
#include <unordered_map>
#include <map>

#include <sys/stat.h>
#include <sys/types.h>

#include <swap-endian.hh>

using namespace kcov;

#define MERGE_MAGIC   0x4d6f6172 // "Moar"
#define MERGE_VERSION 1

struct line_entry
{
	uint32_t line;
	uint32_t hits;
};

struct file_data
{
	uint32_t magic;
	uint32_t version;
	uint32_t checksum;
	uint32_t size;
	uint32_t n_entries;
	uint32_t file_name_offset;

	struct line_entry entries[];
};

// Unit test stuff
namespace merge_parser
{
	class marshal;
	class output;
}

class MergeParser : public IFileParser,
	public IFileParser::ILineListener,
	public IFileParser::IFileListener,
	public IWriter
{
public:
	friend class merge_parser::marshal;
	friend class merge_parser::output;

	MergeParser(IFileParser &localParser,
			const std::string &baseDirectory,
			const std::string &outputDirectory) :
		m_baseDirectory(baseDirectory),
		m_outputDirectory(outputDirectory)
	{
		localParser.registerFileListener(*this);
		localParser.registerLineListener(*this);
	}

	// From IFileParser
	virtual bool addFile(const std::string &filename, struct phdr_data_entry *phdr_data)
	{
		return true;
	}

	virtual void registerLineListener(IFileParser::ILineListener &listener)
	{
		m_listeners.push_back(&listener);
	}


	// Unused ...
	virtual void registerFileListener(IFileParser::IFileListener &listener)
	{
	}

	virtual bool parse()
	{
		return true;
	}

	virtual uint64_t getChecksum()
	{
		return 0;
	}

	// ... to here
	virtual unsigned int matchParser(const std::string &filename, uint8_t *data, size_t dataSize)
	{
		return match_none;
	}


	// From IFileParser::ILineListener
	void onLine(const std::string &filename, unsigned int lineNr,
					unsigned long addr)
	{
		// Nothing to do in that case
		if (!file_exists(filename))
			return;

		LineId key(filename, lineNr);
		File *file;

		file = m_files[filename];
		if (!file) {
			file = new File(filename);
			m_files[filename] = file;
		}

		file->addLine(lineNr, addr);

		for (const auto &it : m_listeners)
			it->onLine(filename, lineNr, addr);
	}

	// From IFileParser::IFileListener
	void onFile(const std::string &file, enum FileFlags flags)
	{
	}


	// From IWriter
	void onStartup()
	{
		mkdir(m_baseDirectory.c_str(), 0755);
		mkdir(m_outputDirectory.c_str(), 0755);
		mkdir(fmt("%s/metadata", m_outputDirectory.c_str()).c_str(), 0755);
	}

	void onStop()
	{
	}

	void write()
	{
		/* Produce something like
		 *
		 *   /tmp/kcov/calc/metadata/4f332bca
		 *   /tmp/kcov/calc/metadata/cd9932a1
		 *
		 * For all the files we've covered. The output filename comes from a hash of
		 * the input filename.
		 */
		for (const auto &it : m_files) {
			const struct file_data *fd = marshalFile(it.second->m_filename);
			uint32_t crc = crc32((const void *)it.second->m_filename.c_str(), it.second->m_filename.size());
			std::string name = fmt("%08x", crc);

			write_file((const void *)fd, be_to_host<uint32_t>(fd->size), "%s/metadata/%s",
					m_outputDirectory.c_str(), name.c_str()
					);

			free((void *)fd);
		}
	}


	void stop()
	{
	}


private:
	const struct file_data *marshalFile(const std::string &filename)
	{
		File *file = m_files[filename];

		if (!file)
			return nullptr;

		// Header + each line + the filename
		size_t size = sizeof(struct file_data) +
				file->m_lines.size() * sizeof(struct line_entry) +
				file->m_filename.size() + 1;
		struct file_data *out = (struct file_data *)xmalloc(size);

		out->magic = to_be<uint32_t>(MERGE_MAGIC);
		out->version = to_be<uint32_t>(MERGE_VERSION);
		out->checksum = to_be<uint32_t>(file->m_checksum);
		out->size = to_be<uint32_t>(size);
		out->n_entries = to_be<uint32_t>(file->m_lines.size());

		struct line_entry *p = out->entries;

		for (auto it = file->m_lines.begin();
				it != file->m_lines.end();
				++it) {
			p->line = to_be<uint32_t>(it->first);
			// FIXME! The number of hits, should probably be a bitmask
			p->hits = to_be<uint32_t>(it->second.size());

			p++;
		}

		char *p_name = (char *)p;
		// Allocated with the terminator above
		strcpy(p_name, file->m_filename.c_str());
		out->file_name_offset = to_be<uint32_t>(p_name - (char *)out);

		return out;
	}

	typedef std::map<unsigned long, unsigned int> AddrMap_t;
	typedef std::map<unsigned int, AddrMap_t> LineAddrMap_t;

	class File
	{
	public:
		File(const std::string &filename) :
			m_filename(filename)
		{
			void *data;
			size_t size;

			uint64_t ts = get_file_timestamp(filename.c_str());

			data = read_file(&size, "%s", filename.c_str());
			panic_if(!data,
					"File %s exists, but can't be read???", filename.c_str());
			m_checksum = crc32(data, size) ^ crc32((const void *)&ts, sizeof(ts));

			free((void *)data);
		}

		void addLine(unsigned int lineNr, unsigned long addr)
		{
			m_lines[lineNr][addr]++;
		}

		std::string m_filename;
		LineAddrMap_t m_lines;
		uint32_t m_checksum;
	};

	// All files in the current coverage session
	std::unordered_map<std::string, File *> m_files;

	std::list<IFileParser::ILineListener *> m_listeners;
	const std::string m_baseDirectory;
	const std::string m_outputDirectory;
};
