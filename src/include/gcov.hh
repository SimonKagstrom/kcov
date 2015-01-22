#include <stdint.h>
#include <string>
#include <vector>
#include <unordered_map>

struct header;

namespace kcov
{
	class GcovParser
	{
	public:
		/**
		 * Parse the gcov file.
		 *
		 * @return true if the file was OK, false otherwise
		 */
		bool parse();

	protected:
		GcovParser(const uint8_t *data, size_t dataSize);

		virtual ~GcovParser();

		virtual bool onRecord(const struct header *header, const uint8_t *data) = 0;

		bool verify();

		const uint8_t *readString(const uint8_t *p, std::string &out);

		// Convenience when using 32-bit pointers
		const int32_t *readString(const int32_t *p, std::string &out);


		const uint8_t *padPointer(const uint8_t *p);

	private:
		const uint8_t *m_data;
		size_t m_dataSize;
	};

	// Specialized parser for gcno files
	class GcnoParser : public GcovParser
	{
	public:
		// Holder-class for fn/bb -> file/line
		class BasicBlockMapping
		{
		public:
			friend GcnoParser;

			const int32_t m_function;
			const int32_t m_basicBlock;
			const std::string m_file;
			const int32_t m_line;

			BasicBlockMapping(const BasicBlockMapping &other);

		private:
			BasicBlockMapping(int32_t function, int32_t basicBlock,
					const std::string &file, int32_t line);
		};

		// Holder-class for arcs between blocks
		class Arc
		{
		public:
			friend GcnoParser;

			const int32_t m_function;
			const int32_t m_srcBlock;
			const int32_t m_dstBlock;

			Arc(const Arc &other);

		private:
			Arc(int32_t function, int32_t srcBlock, int32_t dstBlock);
		};

		typedef std::vector<BasicBlockMapping> BasicBlockList_t;
		typedef std::vector<Arc> ArcList_t;



		GcnoParser(const uint8_t *data, size_t dataSize);

		/* Return a reference to the basic blocks to file/line in the file.
		 * Empty if parse() hasn't been called.
		 *
		 * @return the basic blocks
		 */
		const BasicBlockList_t &getBasicBlocks();

		/**
		 * Return a reference to the arcs in the file. Empty if parse() hasn't
		 * been called.
		 *
		 * @return the arcs
		 */
		const ArcList_t &getArcs();

	protected:
		bool onRecord(const struct header *header, const uint8_t *data);

	private:
		// Handler for record types
		void onAnnounceFunction(const struct header *header, const uint8_t *data);
		void onBlocks(const struct header *header, const uint8_t *data);
		void onLines(const struct header *header, const uint8_t *data);
		void onArcs(const struct header *header, const uint8_t *data);

		std::string m_file;
		std::string m_function;
		int32_t m_functionId;
		BasicBlockList_t m_basicBlocks;
		ArcList_t m_arcs;
	};

	class GcdaParser : public GcovParser
	{
	public:
		GcdaParser(const uint8_t *data, size_t dataSize);

		size_t countersForFunction(int32_t function);

		int64_t getCounter(int32_t function, int32_t counter);

	protected:
		bool onRecord(const struct header *header, const uint8_t *data);

		void onAnnounceFunction(const struct header *header, const uint8_t *data);

		void onCounterBase(const struct header *header, const uint8_t *data);

		typedef std::vector<int64_t> CounterList_t;
		typedef std::unordered_map<int32_t, CounterList_t> FunctionToCountersMap_t;

		int32_t m_functionId;
		FunctionToCountersMap_t m_functionToCounters;
	};
}
