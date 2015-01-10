#pragma once

#include <string>
#include <list>
#include <vector>

namespace kcov
{
	class IConfiguration
	{
	public:
		typedef enum
		{
			MODE_COLLECT_ONLY       = 1,
			MODE_REPORT_ONLY        = 2,
			MODE_COLLECT_AND_REPORT = 3,
		} RunMode_t;

		class IListener
		{
		public:
			virtual ~IListener()
			{
			}

			/**
			 * Callback for configuration changes.
			 *
			 * Listeners will receive this call on each configuration change. The
			 * call is made from a configuration thread, so watch out with races.
			 *
			 * @param key the name of the setting
			 */
			virtual void onConfigurationChanged(const std::string &key) = 0;
		};
		typedef std::vector<IListener *> ConfigurationListener_t;


		virtual ~IConfiguration() {}

		virtual void printUsage() = 0;


		/**
		 * Return a value as a string.
		 *
		 * Will panic if the key is not present.
		 *
		 * @param key the key to lookup
		 *
		 * @return the string value
		 */
		virtual const std::string &keyAsString(const std::string &key) = 0;

		/**
		 * Return a value as an integer.
		 *
		 * Will panic if the key is not present.
		 *
		 * @param key the key to lookup
		 *
		 * @return the integer value
		 */
		virtual int keyAsInt(const std::string &key) = 0;

		/**
		 * Return a value as a string list.
		 *
		 * Will panic if the key is not present.
		 *
		 * @param key the key to lookup
		 *
		 * @return the values as a list, potentially empty
		 */
		virtual const std::vector<std::string> &keyAsList(const std::string &key) = 0;


		virtual const std::string &getOutDirectory() = 0;

		virtual const std::string &getBinaryName() = 0;

		virtual const std::string &getBinaryPath() = 0;

		virtual const char **getArgv() = 0;

		virtual unsigned int getArgc() = 0;

		virtual const std::string& getNewPathPrefix() = 0;
		virtual const std::string& getOriginalPathPrefix() = 0;


		/**
		 * Register a configuration-changed listener
		 *
		 * @param listener the listener to call on changes
		 * @param keys the keys to listen for
		 */
		virtual void registerListener(IListener &listener, const std::vector<std::string> &keys) = 0;


		virtual bool parse(unsigned int argc, const char *argv[]) = 0;


		static IConfiguration &getInstance();
	};
}
