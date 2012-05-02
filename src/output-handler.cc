#include <output-handler.hh>
#include <writer.hh>
#include <configuration.hh>
#include <reporter.hh>
#include <utils.hh>

#include <thread>
#include <list>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

namespace kcov
{
	class OutputHandler : public IOutputHandler
	{
	public:
		OutputHandler(IReporter &reporter) : m_reporter(reporter)
		{
			IConfiguration &conf = IConfiguration::getInstance();

			m_baseDirectory = conf.getOutDirectory();
			m_outDirectory = m_baseDirectory + conf.getBinaryName() + "/";
			m_dbFileName = m_outDirectory + "coverage.db";
			m_summaryDbFileName = m_outDirectory + "summary.db";
		}

		std::string getBaseDirectory()
		{
			return m_baseDirectory;
		}


		std::string getOutDirectory()
		{
			return m_outDirectory;
		}

		void registerWriter(IWriter &writer)
		{
			m_writers.push_back(&writer);
		}

		void start()
		{
			size_t sz;

			mkdir(m_baseDirectory.c_str(), 0755);
			mkdir(m_outDirectory.c_str(), 0755);

			void *data = read_file(&sz, m_dbFileName.c_str());

			if (data) {
				m_reporter.unMarshal(data, sz);

				free(data);
			}

			for (WriterList_t::iterator it = m_writers.begin();
					it != m_writers.end();
					it++)
				(*it)->onStartup();

			m_thread = new std::thread(threadMainStatic, this);
			m_cv.notify_all();
		}

		void stop()
		{
			m_stop = true;
			m_cv.notify_all();

			if (m_thread)
				m_thread->join();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			size_t sz;
			void *data = m_reporter.marshal(&sz);

			if (data)
				write_file(data, sz, m_dbFileName.c_str());

			free(data);

			for (WriterList_t::iterator it = m_writers.begin();
					it != m_writers.end();
					it++)
				(*it)->onStop();
		}

	private:

		void threadMain()
		{
			while (!m_stop) {
				std::unique_lock<std::mutex> lk(m_mutex);

				m_cv.wait_for(lk, std::chrono::milliseconds(1003));

				for (WriterList_t::iterator it = m_writers.begin();
						it != m_writers.end();
						it++)
					(*it)->write();
			}
		}

		static void threadMainStatic(OutputHandler *pThis)
		{
			pThis->threadMain();
		}


		typedef std::list<IWriter *> WriterList_t;

		IReporter &m_reporter;

		std::string m_outDirectory;
		std::string m_baseDirectory;
		std::string m_dbFileName;
		std::string m_summaryDbFileName;

		WriterList_t m_writers;
		std::thread *m_thread;
		std::condition_variable m_cv;
		std::mutex m_mutex;
		bool m_stop;
	};

	IOutputHandler &IOutputHandler::create(IReporter &reporter)
	{
		static OutputHandler *instance;

		if (!instance)
			instance = new OutputHandler(reporter);

		return *instance;
	}
}
