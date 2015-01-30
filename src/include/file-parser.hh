#pragma once

#include <manager.hh>

#include <sys/types.h>
#include <stdint.h>

#include <string>
#include <vector>

struct phdr_data_entry;

namespace kcov
{
	class IFilter;

	class IFileParser
	{
	public:
		enum FileFlags
		{
			FLG_NONE = 0,
			FLG_TYPE_SOLIB = 1,
			FLG_TYPE_COVERAGE_DATA = 2, //< Typically gcov data files
		};

		enum PossibleHits
		{
			HITS_SINGLE,     //< Yes/no (merge-parser)
			HITS_LIMITED,    //< E.g., multiple branches
			HITS_UNLIMITED,  //< Accumulated (Python/bash)
		};

		/**
		 * Holder class for address segments
		 */
		class Segment
		{
		public:
			Segment(uint64_t paddr, uint64_t vaddr, uint64_t size) :
				m_paddr(paddr), m_vaddr(vaddr), m_size(size)
			{
			}

			/**
			 * Check if an address is contained within this segment
			 *
			 * @param addr the address to check
			 *
			 * @return true if valid
			 */
			bool addressIsWithinSegment(uint64_t addr) const
			{
				return addr >= m_paddr && addr < m_paddr + m_size;
			}

			/**
			 * Adjust an address with the segment.
			 *
			 * @param addr the address to adjust
			 *
			 * @return the new address
			 */
			uint64_t adjustAddress(uint64_t addr) const
			{
				if (addressIsWithinSegment(addr))
					return addr - m_paddr + m_vaddr;

				return addr;
			}

			uint64_t getBase() const
			{
				return m_vaddr;
			}

			size_t getSize() const
			{
				return m_size;
			}

		private:
			const uint64_t m_paddr;
			const uint64_t m_vaddr;
			const size_t m_size;
		};
		typedef std::vector<Segment> SegmentList_t;

		/**
		 * Holder class for files (e.g., ELF binaries)
		 */
		class File
		{
		public:
			File(const std::string &filename, const enum FileFlags flags = FLG_NONE) :
				m_filename(filename), m_flags(flags)
			{
			}

			File(const std::string &filename, const enum FileFlags flags, const SegmentList_t &segments) :
				m_filename(filename), m_flags(flags), m_segments(segments)
			{
			}

			const std::string m_filename;
			const enum FileFlags m_flags;
			const SegmentList_t m_segments;
		};

		virtual ~IFileParser() {}

		/**
		 * Listener for lines (lines in source files)
		 *
		 * This is the main way the (file,lineNr) -> address map is handled.
		 */
		class ILineListener
		{
		public:
			virtual void onLine(const std::string &file, unsigned int lineNr,
					uint64_t addr) = 0;
		};

		/**
		 * Listener for added files (typically an ELF binary)
		 */
		class IFileListener
		{
		public:
			virtual void onFile(const File &file) = 0;
		};

		/**
		 * Add a file to the parser
		 *
		 * @param filename the filename to add
		 * @param phdr_data base address data for solibs
		 *
		 * @return true if the file could be added, false otherwise
		 */
		virtual bool addFile(const std::string &filename, struct phdr_data_entry *phdr_data = 0) = 0;

		/**
		 * Register a listener for source lines.
		 *
		 * Will be called when new source file/line pairs are found
		 *
		 * @param listener the listener
		 */
		virtual void registerLineListener(ILineListener &listener) = 0;

		/**
		 * Register a listener for coveree files.
		 *
		 * Will be called when new ELF binary (typically) is added
		 *
		 * @param listener the listener
		 */
		virtual void registerFileListener(IFileListener &listener) = 0;

		/**
		 * Parse the added files
		 *
		 * @return true if parsing could be done
		 */
		virtual bool parse() = 0;

		/**
		 * Get the checksum of the main file (not solibs)
		 *
		 * @return the file checksum
		 */
		virtual uint64_t getChecksum() = 0;

		/**
		 * Get the name of the parser
		 *
		 * @return the name
		 */
		virtual std::string getParserType() = 0;


		/**
		 * Return if this parser is of the multiple type (i.e., relying on
		 * breakpoints which are cleared after hit, but can have branches),
		 * or if every address can occur multiple times, or if only
		 * covered/non-covered is possible.
		 *
		 * @return the possible hits of this parser
		 */
		virtual enum PossibleHits maxPossibleHits() = 0;

		/**
		 * See if a particular file can be matched with this parser.
		 *
		 * Should return how well this parser fits, the higher the better
		 *
		 * @param filename the name of the file
		 * @param data the first few bytes of the file
		 * @param dataSize the size of @a data
		 */
		virtual unsigned int matchParser(const std::string &filename, uint8_t *data, size_t dataSize) = 0;

		// Setup once the parser has been chosen
		virtual void setupParser(IFilter *filter) = 0;
	};


	/**
	 * Manager class for getting one of multiple parsers, which can
	 * match different file types.
	 */
	class IParserManager
	{
	public:
		virtual ~IParserManager()
		{
		}

		virtual void registerParser(IFileParser &parser) = 0;

		virtual IFileParser *matchParser(const std::string &file) = 0;

		static IParserManager &getInstance();
	};
}
