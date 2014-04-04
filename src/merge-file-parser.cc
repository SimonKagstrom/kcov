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

#include <swap-endian.hh>

using namespace kcov;

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
	uint32_t n_entries;
	uint32_t file_name_offset;

	struct line_entry entries[];
};

// Unit test stuff
namespace merge_parser
{
	class marshal;
}

class MergeParser : public IFileParser,
	public IFileParser::ILineListener,
	public IFileParser::IFileListener,
	public IWriter
{
public:
	friend class merge_parser::marshal;

	MergeParser(IFileParser &localParser)
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
	void onLine(const std::string &file, unsigned int lineNr,
					unsigned long addr)
	{
		LineId key(file, lineNr);

		m_localEntries[key][addr]++;

		for (const auto &it : m_listeners)
			it->onLine(file, lineNr, addr);
	}

	// From IFileParser::IFileListener
	void onFile(const std::string &file, enum FileFlags flags)
	{
	}


	// From IWriter
	void onStartup()
	{
	}

	void onStop()
	{
	}

	void write()
	{
	}


	void stop()
	{
	}


private:
	const file_data *marshalFile(const std::string &name)
	{
		return NULL;
	}


	typedef std::map<unsigned long, unsigned int> AddrMap_t;

	// Entries for this particular coverage session, and for all globally
	std::unordered_map<LineId, AddrMap_t, LineIdHash> m_localEntries;
	std::unordered_map<LineId, unsigned int, LineIdHash> m_globalEntries;

	std::list<IFileParser::ILineListener *> m_listeners;
};
