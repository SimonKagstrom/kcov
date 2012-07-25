#include <output-handler.hh>
#include <writer.hh>
#include <configuration.hh>
#include <reporter.hh>
#include <elf.hh>
#include <utils.hh>

#include <thread>
#include <list>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <pthread.h>

namespace kcov
{
	class OutputHandler : public IOutputHandler, IElf::IFileListener
	{
	public:
		OutputHandler(IReporter &reporter) : m_reporter(reporter)
		{
			IConfiguration &conf = IConfiguration::getInstance();

			m_baseDirectory = conf.getOutDirectory();
			m_outDirectory = m_baseDirectory + conf.getBinaryName() + "/";
			m_dbFileName = m_outDirectory + "coverage.db";
			m_summaryDbFileName = m_outDirectory + "summary.db";

			m_threadStopped = false;
			m_stop = false;

			mkdir(m_baseDirectory.c_str(), 0755);
			mkdir(m_outDirectory.c_str(), 0755);

			IElf::getInstance().registerFileListener(*this);
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

		// From IElf::IFileListener
		void onFile(const char *file, bool isSolib)
		{
			// Only unmarshal the main file
			if (isSolib)
				return;

			size_t sz;

			void *data = read_file(&sz, m_dbFileName.c_str());

			if (data) {
				if (!m_reporter.unMarshal(data, sz))
					warning("Can't unmarshal %s\n", m_dbFileName.c_str());
			}

			free(data);
		}

		void start()
		{
			for (WriterList_t::iterator it = m_writers.begin();
					it != m_writers.end();
					it++)
				(*it)->onStartup();

			panic_if (pthread_create(&m_thread, NULL, threadMainStatic, this) < 0,
					"Can't create thread");
		}

		void stop()
		{
			m_stop = true;

			for (WriterList_t::iterator it = m_writers.begin();
					it != m_writers.end();
					it++)
				(*it)->stop();

			// Wait for half a second
			for (unsigned int i = 0; i < 50; i++)
			{
				if (m_threadStopped)
					break;
				mdelay(10);
			}

			for (WriterList_t::iterator it = m_writers.begin();
					it != m_writers.end();
					it++)
				(*it)->write();

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
			sigset_t set;

			sigemptyset(&set);
			sigaddset(&set, SIGINT);
			sigaddset(&set, SIGTERM);

			// Block INT and TERM to avoid Ctrl-C ending up in this thread
			sigprocmask(SIG_BLOCK, &set, NULL);
			while (!m_stop) {
				for (WriterList_t::iterator it = m_writers.begin();
						it != m_writers.end();
						it++)
					(*it)->write();

				sleep(1);
			}

			m_threadStopped = true;
		}

		static void *threadMainStatic(void *pThis)
		{
			OutputHandler *p = (OutputHandler *)pThis;

			p->threadMain();

			return NULL;
		}


		typedef std::list<IWriter *> WriterList_t;

		IReporter &m_reporter;

		std::string m_outDirectory;
		std::string m_baseDirectory;
		std::string m_dbFileName;
		std::string m_summaryDbFileName;

		WriterList_t m_writers;
		pthread_t m_thread;
		bool m_stop;
		bool m_threadStopped;
	};

	static OutputHandler *instance;
	IOutputHandler &IOutputHandler::create(IReporter &reporter)
	{
		if (!instance)
			instance = new OutputHandler(reporter);

		return *instance;
	}

	IOutputHandler &IOutputHandler::getInstance()
	{
		return *instance;
	}
}
