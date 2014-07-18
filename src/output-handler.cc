#include <output-handler.hh>
#include <writer.hh>
#include <configuration.hh>
#include <reporter.hh>
#include <file-parser.hh>
#include <utils.hh>

#include <thread>
#include <list>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>

namespace kcov
{
	class OutputHandler : public IOutputHandler, IFileParser::IFileListener
	{
	public:
		OutputHandler(IFileParser &parser, IReporter &reporter) :
			m_reporter(reporter),
			m_unmarshalSize(0),
			m_unmarshalData(NULL)
		{
			IConfiguration &conf = IConfiguration::getInstance();

			m_baseDirectory = conf.getOutDirectory();
			m_outDirectory = m_baseDirectory + conf.getBinaryName() + "/";
			m_dbFileName = m_outDirectory + "/coverage.db";
			m_summaryDbFileName = m_outDirectory + "/summary.db";

			mkdir(m_baseDirectory.c_str(), 0755);
			mkdir(m_outDirectory.c_str(), 0755);

			parser.registerFileListener(*this);
		}

		~OutputHandler()
		{
			stop();
			free(m_unmarshalData);
		}

		const std::string &getBaseDirectory()
		{
			return m_baseDirectory;
		}


		const std::string &getOutDirectory()
		{
			return m_outDirectory;
		}

		void registerWriter(IWriter &writer)
		{
			m_writers.push_back(&writer);
		}

		// From IElf::IFileListener
		void onFile(const std::string &file, enum IFileParser::FileFlags flags)
		{
			// Only unmarshal the main file
			if (flags & IFileParser::FLG_TYPE_SOLIB)
				return;

			if (!m_unmarshalData)
				m_unmarshalData = read_file(&m_unmarshalSize, "%s", m_dbFileName.c_str());

			if (m_unmarshalData) {
				if (!m_reporter.unMarshal(m_unmarshalData, m_unmarshalSize))
					kcov_debug(INFO_MSG, "Can't unmarshal %s\n", m_dbFileName.c_str());
			}
		}

		void start()
		{
			for (WriterList_t::const_iterator it = m_writers.begin();
					it != m_writers.end();
					++it)
				(*it)->onStartup();
		}

		void stop()
		{
			size_t sz;
			void *data = m_reporter.marshal(&sz);

			if (data)
				write_file(data, sz, "%s", m_dbFileName.c_str());

			free(data);

			for (WriterList_t::const_iterator it = m_writers.begin();
					it != m_writers.end();
					++it)
				(*it)->onStop();

			// Produce output after stop if anyone yields new data in onStop()
			produce();
		}

		void produce()
		{
			for (WriterList_t::const_iterator it = m_writers.begin();
					it != m_writers.end();
					++it)
				(*it)->write();
		}


	private:
		typedef std::list<IWriter *> WriterList_t;

		IReporter &m_reporter;

		std::string m_outDirectory;
		std::string m_baseDirectory;
		std::string m_dbFileName;
		std::string m_summaryDbFileName;

		WriterList_t m_writers;

		size_t m_unmarshalSize;
		void *m_unmarshalData;
	};

	static OutputHandler *instance;
	IOutputHandler &IOutputHandler::create(IFileParser &parser, IReporter &reporter)
	{
		if (!instance)
			instance = new OutputHandler(parser, reporter);

		return *instance;
	}

	IOutputHandler &IOutputHandler::getInstance()
	{
		return *instance;
	}
}
