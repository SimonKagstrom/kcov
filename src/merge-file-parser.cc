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
#include <dirent.h>

#include <swap-endian.hh>

using namespace kcov;

#define MERGE_MAGIC   0x4d6f6172 // "Moar"
#define MERGE_VERSION 1

struct line_entry
{
	uint32_t line;
	uint32_t address_start;
	uint32_t n_addresses;
};

struct file_data
{
	uint32_t magic;
	uint32_t version;
	uint32_t checksum;
	uint32_t size;
	uint32_t n_entries;
	uint32_t address_table_offset;
	uint32_t file_name_offset;

	struct line_entry entries[];
};

// Unit test stuff
namespace merge_parser
{
	class marshal;
	class output;
	class input;
}

class MergeParser : public IFileParser,
	public IFileParser::ILineListener,
	public IFileParser::IFileListener,
	public ICollector::IListener,
	public ICollector,
	public IWriter
{
public:
	friend class merge_parser::marshal;
	friend class merge_parser::output;
	friend class merge_parser::input;

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
		m_lineListeners.push_back(&listener);
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

	// From ICollector::IListener
	void onAddress(unsigned long addr, unsigned long hits)
	{
		File *file = m_filesByAddress[addr];

		if (!file)
			return;

		file->registerHits(addr, hits);

		for (const auto &it : m_collectorListeners) {
			it->onAddress(addr, hits);
		}
	}

	// From IFileParser::ILineListener
	void onLine(const std::string &filename, unsigned int lineNr,
					unsigned long addr)
	{
		// Nothing to do in that case
		if (!file_exists(filename))
			return;

		File *file;

		file = m_files[filename];
		if (!file) {
			file = new File(filename);
			m_files[filename] = file;

			// This should be part of the merged data
			m_fileHashes[file->m_checksum] = file->m_filename;
		}

		// Mark as a local file
		file->setLocal();
		file->addLine(lineNr, addr & ~(1ULL << 63));

		for (const auto &it : m_lineListeners) {
			it->onLine(filename, lineNr, addr);

			// Record this for the collector hits
			m_filesByAddress[addr] = file;
		}
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

		parseStoredData();
	}

	void onStop()
	{
	}

	void write()
	{
		parseStoredData();

		/* Produce something like
		 *
		 *   /tmp/kcov/calc/metadata/4f332bca
		 *   /tmp/kcov/calc/metadata/cd9932a1
		 *
		 * For all the files we've covered. The output filename comes from a hash of
		 * the input filename.
		 */
		for (const auto &it : m_files) {
			// Only marshal local files
			if (!it.second->m_local)
				continue;

			const struct file_data *fd = marshalFile(it.second->m_filename);
			uint32_t crc = crc32((const void *)it.second->m_filename.c_str(), it.second->m_filename.size());
			std::string name = fmt("%08x", crc);

			write_file((const void *)fd, be_to_host<uint32_t>(fd->size), "%s/metadata/%s",
					m_outputDirectory.c_str(), name.c_str()
					);

			free((void *)fd);
		}
	}


	// From ICollector
	virtual void registerListener(ICollector::IListener &listener)
	{
		m_collectorListeners.push_back(&listener);
	}

	virtual int run(const std::string &filename)
	{
		// Not used
		panic("Should not call run here");

		return 0;
	}


private:
	void parseStoredData()
	{
		DIR *dir;
		struct dirent *de;

		dir = opendir(m_baseDirectory.c_str());
		panic_if(!dir,
				"Can't open directory %s\n", m_baseDirectory.c_str());

		// Unmarshal and parse all metadata
		for (de = readdir(dir); de; de = readdir(dir)) {
			std::string cur = m_baseDirectory + de->d_name;

			// ... except for the current coveree
			if (cur == m_outputDirectory)
				continue;

			parseDirectory(cur);
		}
		closedir(dir);
	}

	void parseDirectory(const std::string &dirName)
	{
		DIR *dir;
		struct dirent *de;
		std::string metadataDirName = dirName + "/metadata";

		dir = opendir(metadataDirName.c_str());
		// Can occur naturally
		if(!dir)
			return;

		// Read all metadata from the directory
		for (de = readdir(dir); de; de = readdir(dir))
			parseOne(metadataDirName, de->d_name);

		closedir(dir);
	}

	void parseOne(const std::string &metadataDirName,
			const std::string &curFile)
	{
		size_t size;

		// Not as hash?
		if (!string_is_integer(curFile, 16))
			return;

		// We don't have this file, or the file is older/newer
		if (m_fileHashes.find(string_to_integer(curFile, 16)) == m_fileHashes.end())
			return;


		struct file_data *fd = (struct file_data *)read_file(&size, "%s/%s",
				metadataDirName.c_str(), curFile.c_str());
		if (!fd)
			return;

		if (size >= sizeof(struct file_data) &&
				unMarshalFile(fd)) {
			parseFileData(fd);
		}

		free(fd);
	}

	void parseFileData(struct file_data *fd)
	{
		std::string filename((const char *)fd + fd->file_name_offset);

		// Do something

		File *file;

		file = m_files[filename];
		if (!file) {
			file = new File(filename);
			m_files[filename] = file;

			// This should be part of the merged data
			m_fileHashes[file->m_checksum] = file->m_filename;
		} else {
			// Checksum doesn't match, ignore this file
			if (file->m_local &&
					file->m_checksum != fd->checksum)
				return;
		}

		uint64_t *addrTable = (uint64_t *)((char *)fd + fd->address_table_offset);
		for (unsigned i = 0; i < fd->n_entries; i++) {
			uint32_t lineNr = fd->entries[i].line;

			for (unsigned ia = 0; ia < fd->entries[i].n_addresses; ia++) {
				uint64_t addr = addrTable[fd->entries[i].address_start + ia];
				bool hit = (addr & (1ULL << 63));

				file->addLine(lineNr, addr);

				for (const auto &it : m_lineListeners)
					it->onLine(filename, lineNr, addr & ~(1ULL << 63));

				// Register and report the hit
				if (hit) {
					file->registerHits(lineNr, 1);

					for (const auto &itC : m_collectorListeners)
						itC->onAddress(addr, 1);
				}
			}
		}
	}

	const struct file_data *marshalFile(const std::string &filename)
	{
		File *file = m_files[filename];

		if (!file)
			return nullptr;

		uint32_t n_addrs = 0;
		for (auto it = file->m_lines.begin();
				it != file->m_lines.end();
				++it) {
			n_addrs += it->second.size();
		}

		// Header + each line + the filename
		size_t size = sizeof(struct file_data) +
				file->m_lines.size() * sizeof(struct line_entry) +
				n_addrs * sizeof(uint64_t) +
				file->m_filename.size() + 1;
		struct file_data *out = (struct file_data *)xmalloc(size);

		out->magic = to_be<uint32_t>(MERGE_MAGIC);
		out->version = to_be<uint32_t>(MERGE_VERSION);
		out->checksum = to_be<uint32_t>(file->m_checksum);
		out->size = to_be<uint32_t>(size);
		out->n_entries = to_be<uint32_t>(file->m_lines.size());

		struct line_entry *p = out->entries;

		// Point to address table
		uint64_t *addrTable = (uint64_t *)((char *)p + sizeof(struct line_entry) * file->m_lines.size());
		uint32_t tableOffset = 0;

		for (auto it = file->m_lines.begin();
				it != file->m_lines.end();
				++it) {
			uint32_t line = it->first;
			p->line = to_be<uint32_t>(line);

			p->address_start = to_be<uint32_t>(tableOffset);
			p->n_addresses = to_be<uint32_t>(it->second.size());
			for (const auto &itAddr : it->second) {
				uint64_t addr = itAddr.first;

				if (file->m_addrHits[addr])
					addr |= (1ULL << 63);

				addrTable[tableOffset] = to_be<uint64_t>(addr);

				tableOffset++;
			}

			p++;
		}

		uint32_t tableStart = (uint32_t)((char *)addrTable - (char *)out);
		out->address_table_offset = to_be<uint32_t>(tableStart);
		char *p_name = (char *)out + tableStart + tableOffset * sizeof(uint64_t);

		// Allocated with the terminator above
		strcpy(p_name, file->m_filename.c_str());
		out->file_name_offset = to_be<uint32_t>(p_name - (char *)out);

		return out;
	}

	bool unMarshalFile(struct file_data *fd)
	{
		fd->magic = be_to_host<uint32_t>(fd->magic);
		fd->version = be_to_host<uint32_t>(fd->version);
		fd->checksum = be_to_host<uint32_t>(fd->checksum);
		fd->size = be_to_host<uint32_t>(fd->size);
		fd->n_entries = be_to_host<uint32_t>(fd->n_entries);
		fd->file_name_offset = be_to_host<uint32_t>(fd->file_name_offset);
		fd->address_table_offset = be_to_host<uint32_t>(fd->address_table_offset);

		if (fd->magic != MERGE_MAGIC)
			return false;

		if (fd->version != MERGE_VERSION)
			return false;

		struct line_entry *p = fd->entries;

		// Unmarshal entries...
		for (unsigned i = 0; i < fd->n_entries; i++) {
			p->line = be_to_host<uint32_t>(p->line);
			p->n_addresses = be_to_host<uint32_t>(p->n_addresses);
			p->address_start = be_to_host<uint32_t>(p->address_start);

			p++;
		}

		// ... and the address table
		uint64_t *addressTable = (uint64_t *)((char *)fd + fd->address_table_offset);
		for (unsigned i = 0;
				i < (fd->file_name_offset - fd->address_table_offset) / sizeof(uint64_t);
				i++)
			addressTable[i] = be_to_host<uint64_t>(addressTable[i]);

		return true;
	}

	typedef std::map<unsigned long, unsigned int> AddrMap_t;
	typedef std::map<unsigned int, AddrMap_t> LineAddrMap_t;

	class File
	{
	public:
		File(const std::string &filename) :
			m_filename(filename),
			m_local(false)
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

		void setLocal()
		{
			m_local = true;
		}

		void addLine(unsigned int lineNr, uint64_t addr)
		{
			m_lines[lineNr][addr]++;
		}

		void registerHits(uint64_t addr, unsigned int hits)
		{
			m_addrHits[addr] += hits;
		}

		std::string m_filename;
		LineAddrMap_t m_lines;
		AddrMap_t m_addrHits;
		uint32_t m_checksum;
		bool m_local;
	};

	// All files in the current coverage session
	std::unordered_map<std::string, File *> m_files;
	std::unordered_map<uint32_t, std::string> m_fileHashes;
	std::unordered_map<unsigned long, File *> m_filesByAddress;

	std::vector<IFileParser::ILineListener *> m_lineListeners;
	const std::string m_baseDirectory;
	const std::string m_outputDirectory;

	std::vector<ICollector::IListener *> m_collectorListeners;
};
