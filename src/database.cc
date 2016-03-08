#include <file-parser.hh>
#include <configuration.hh>
#include <utils.hh>
#include <swap-endian.hh>

#include <unordered_map>
#include "include/database.hh"

using namespace kcov;

#define MAGIC 0x526f7952 /* RoyR */

struct header
{
	uint32_t magic;
	uint32_t n_entries;
	uint64_t entries[];
};

class DatabaseCreator : public IDatabaseCreator,
	IFileParser::IFileListener, IFileParser::ILineListener
{
public:
	DatabaseCreator(IFileParser &parser) :
		m_currentFileChecksum(0)
	{
		parser.registerFileListener(*this);
		parser.registerLineListener(*this);
	}

	void write()
	{
		marshal();
	}

private:
	void onLine(const std::string &file, unsigned int lineNr, uint64_t addr)
	{
		m_addresses[addr]++;
	}

	void onFile(const IFileParser::File &file)
	{
		if (m_currentFileChecksum == file.m_checksum)
			return;

		marshal();
		m_addresses.clear();
		m_currentFileChecksum = file.m_checksum;
	}

	void marshal()
	{
		// Typically the case at startup
		if (m_addresses.empty())
			return;


		size_t dataSize = sizeof(header) + sizeof(uint64_t) * m_addresses.size();
		struct header *hdr = (struct header *)xmalloc(dataSize);

		hdr->magic = to_be<uint32_t>(MAGIC);
		hdr->n_entries = to_be<uint32_t>(m_addresses.size());

		uint64_t *entry = hdr->entries;
		for (std::unordered_map<uint64_t, unsigned int>::iterator it = m_addresses.begin();
				it != m_addresses.end();
				++it) {
			uint64_t cur = it->first;

			*entry = to_be<uint64_t>(cur);
			entry++;
		}

		std::string dir = IConfiguration::getInstance().keyAsString("database-directory");
		std::string outFile = fmt("%s/%llx", dir.c_str(), (unsigned long long)m_currentFileChecksum);

		int rv = write_file(hdr, dataSize, "%s", outFile.c_str());
		if (rv < 0)
			panic("Can't write database file %s", outFile.c_str());

		free(hdr);
	}

	uint64_t m_currentFileChecksum;
	std::unordered_map<uint64_t, unsigned int> m_addresses;
};

IDatabaseCreator &IDatabaseCreator::create(IFileParser &parser)
{
	return *new DatabaseCreator(parser);
}



class DatabaseReader : public IDatabaseReader
{
public:
	DatabaseReader(IFileParser &parser)
	{
	}

	const std::vector<uint64_t> &get(uint64_t checksum)
	{
		std::string dir = IConfiguration::getInstance().keyAsString("database-directory");
		std::string filename = fmt("%s/%llx", dir.c_str(), (unsigned long long)checksum);

		if (!file_exists(filename))
			return m_empty;

		if (!unmarshal(filename))
			return m_empty;

		return m_database;
	}

private:
	bool unmarshal(const std::string &filename)
	{
		size_t sz;
		struct header *hdr = (struct header *)read_file(&sz, "%s", filename.c_str());

		if (!hdr)
			return false;

		// Yes, std::unique_ptr would have been easier here
		if (sz <= sizeof(struct header)) {
			free(hdr);
			return false;
		}

		hdr->magic = be_to_host<uint32_t>(hdr->magic);
		hdr->n_entries = be_to_host<uint32_t>(hdr->n_entries);

		sz -= sizeof(struct header);
		if (hdr->magic != MAGIC || hdr->n_entries != sz/ sizeof(uint64_t)) {
			free(hdr);
			return false;
		}

		// All OK, unmarshal entries and populate the database
		m_database.clear();
		for (uint32_t i = 0; i < hdr->n_entries; i++)
			m_database.push_back(be_to_host<uint64_t>(hdr->entries[i]));

		free(hdr);

		return true;
	}

	std::vector<uint64_t> m_database;
	std::vector<uint64_t> m_empty;
};

IDatabaseReader &IDatabaseReader::create(IFileParser &parser)
{
	return *new DatabaseReader(parser);
}
