#include <file-parser.hh>
#include <engine.hh>
#include <utils.hh>
#include <capabilities.hh>
#include <gcov.hh>
#include <phdr_data.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libelf.h>
#include <dwarf.h>
#include <elfutils/libdw.h>
#include <map>
#include <vector>
#include <string>
#include <configuration.hh>
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#include <link.h>

#include <filter.hh>

using namespace kcov;

enum SymbolType
{
	SYM_NORMAL = 0,
	SYM_DYNAMIC = 1,
};

class ElfInstance : public IFileParser
{
public:
	ElfInstance()
	{
		m_elf = NULL;
		m_filename = "";
		m_checksum = 0;
		m_elfIs32Bit = true;
		m_isMainFile = true;
		m_initialized = false;
		m_filter = NULL;

		IParserManager::getInstance().registerParser(*this);
	}

	virtual ~ElfInstance()
	{
	}

	uint64_t getChecksum()
	{
		return m_checksum;
	}

	std::string getParserType()
	{
		return "ELF";
	}

	bool elfIs64Bit()
	{
		return !m_elfIs32Bit;
	}

	void setupParser(IFilter *filter)
	{
		m_filter = filter;
	}

	enum IFileParser::PossibleHits maxPossibleHits()
	{
		return IFileParser::HITS_LIMITED; // Breakpoints are cleared after a hit
	}

	unsigned int matchParser(const std::string &filename, uint8_t *data, size_t dataSize)
	{
		Elf32_Ehdr *hdr = (Elf32_Ehdr *)data;

		if (memcmp(hdr->e_ident, ELFMAG, strlen(ELFMAG)) == 0)
			return match_perfect;

		return match_none;
	}

	bool addFile(const std::string &filename, struct phdr_data_entry *data)
	{
		if (!m_initialized) {
			/******* Swap debug source root with runtime source root *****/
			m_origRoot = IConfiguration::getInstance().keyAsString("orig-path-prefix");
			m_newRoot  = IConfiguration::getInstance().keyAsString("new-path-prefix");

			panic_if(elf_version(EV_CURRENT) == EV_NONE,
					"ELF version failed\n");
			m_initialized = true;
		}


		m_filename = filename;

		m_buildId.clear();

		m_curSegments.clear();
		m_executableSegments.clear();
		for (uint32_t i = 0; data && i < data->n_segments; i++) {
			struct phdr_data_segment *seg = &data->segments[i];

			m_curSegments.push_back(Segment(seg->paddr, seg->vaddr, seg->size));
		}

		return checkFile();
	}

	bool checkFile()
	{
		struct Elf *elf;
		bool out = true;
		int fd;

		fd = ::open(m_filename.c_str(), O_RDONLY, 0);
		if (fd < 0) {
				error("Cannot open %s\n", m_filename.c_str());
				return false;
		}

		if (!(elf = elf_begin(fd, ELF_C_READ, NULL)) ) {
				error("elf_begin failed on %s\n", m_filename.c_str());
				out = false;
				goto out_open;
		}
		if (elf_kind(elf) == ELF_K_NONE)
			out = false;

		if (m_isMainFile) {
			char *raw;
			size_t sz;

			raw = elf_getident(elf, &sz);

			if (raw && sz > EI_CLASS) {
				ICapabilities &capabilities = ICapabilities::getInstance();

				m_elfIs32Bit = raw[EI_CLASS] == ELFCLASS32;

				if (elfIs64Bit() != machine_is_64bit())
					capabilities.removeCapability("handle-solibs");
				else
					capabilities.addCapability("handle-solibs");
			}

			m_checksum = m_elfIs32Bit ? elf32_checksum(elf) : elf64_checksum(elf);
		}

		elf_end(elf);

out_open:
		close(fd);

		return out;
	}

	bool parse()
	{
		struct stat st;

		if (lstat(m_filename.c_str(), &st) < 0)
			return 0;

		parseOneElf();

		// Gcov data?
		if (IConfiguration::getInstance().keyAsInt("gcov") && !m_gcnoFiles.empty())
			parseGcnoFiles();
		else
			parseOneDwarf();

		for (FileListenerList_t::const_iterator it = m_fileListeners.begin();
				it != m_fileListeners.end();
				++it)
			(*it)->onFile(File(m_filename, m_isMainFile ? IFileParser::FLG_NONE : IFileParser::FLG_TYPE_SOLIB));

		// After the first, all other are solibs
		m_isMainFile = false;

		return true;
	}

	void parseGcnoFiles()
	{
		for (FileList_t::const_iterator it = m_gcnoFiles.begin();
				it != m_gcnoFiles.end();
				++it) {
			const std::string &cur = *it;

			parseOneGcno(cur);
		}
	}

	void parseOneGcno(const std::string &filename)
	{
		size_t sz;
		void *data;

		// The data is freed by the parser
		data = read_file(&sz, "%s", filename.c_str());
		if (!data)
			return;

		// Parse this gcno file!
		GcnoParser parser((uint8_t *)data, sz);

		// Parsing error?
		if (!parser.parse()) {
			warning("Can't parse %s\n", filename.c_str());

			return;
		}

		const GcnoParser::BasicBlockList_t &bbs = parser.getBasicBlocks();

		for (GcnoParser::BasicBlockList_t::const_iterator it = bbs.begin();
				it != bbs.end();
				++it) {
			const GcnoParser::BasicBlockMapping &cur = *it;

			// Report a generated address
			for (LineListenerList_t::const_iterator it = m_lineListeners.begin();
					it != m_lineListeners.end();
					++it)
				(*it)->onLine(cur.m_file, cur.m_line,
						gcovGetAddress(cur.m_file, cur.m_function, cur.m_basicBlock, cur.m_index));
		}
	}

	bool parseOneDwarf()
	{
		Dwarf_Off offset = 0;
		Dwarf_Off last_offset = 0;
		size_t hdr_size;
		Dwarf *dbg;
		int fd;

		fd = ::open(m_filename.c_str(), O_RDONLY, 0);
		if (fd < 0) {
				error("Cannot open %s\n", m_filename.c_str());
				return false;
		}

		/* Initialize libdwarf */
		dbg = dwarf_begin(fd, DWARF_C_READ);

		if (!dbg && m_buildId.length() > 0) {
			/* Look for separate debug info */
			std::string debug_file = std::string("/usr/lib/debug/.build-id/" +
							     m_buildId.substr(0,2) +
							     "/" +
							     m_buildId.substr(2, std::string::npos) +
							     ".debug");

			close(fd);
			fd = ::open(debug_file.c_str(), O_RDONLY, 0);
			if (fd < 0) {
				// Some shared libraries have neither symbols nor build-id files
				if (m_isMainFile) {
					warning("Cannot open %s", debug_file.c_str());
					warning("kcov requires binaries built with -g/-ggdb or a build-id file.");
				}
				return false;
			}

			dbg = dwarf_begin(fd, DWARF_C_READ);
		}

		if (!dbg) {
			kcov_debug(ELF_MSG, "No debug symbols in %s.\n", m_filename.c_str());
			close(fd);
			return false;
		}

		/* Iterate over the headers */
		while (dwarf_nextcu(dbg, offset, &offset, &hdr_size, 0, 0, 0) == 0) {
			Dwarf_Lines* line_buffer;
			Dwarf_Files *file_buffer;
			size_t line_count;
			size_t file_count;
			Dwarf_Die die;
			unsigned int i;

			if (dwarf_offdie(dbg, last_offset + hdr_size, &die) == NULL)
				goto out_err;

			last_offset = offset;

			/* Get the source lines */
			if (dwarf_getsrclines(&die, &line_buffer, &line_count) != 0)
				continue;

			/* And the files */
			if (dwarf_getsrcfiles(&die, &file_buffer, &file_count) != 0)
				continue;

			const char *const *src_dirs;
			size_t ndirs = 0;

			/* Lookup the compilation path */
			if (dwarf_getsrcdirs(file_buffer, &src_dirs, &ndirs) != 0)
				continue;

			if (ndirs == 0)
				continue;

			/* Store them */
			for (i = 0; i < line_count; i++) {
				Dwarf_Line *line;
				int line_nr;
				const char* line_source;
				Dwarf_Word mtime, len;
				bool is_code;
				Dwarf_Addr addr;

				if ( !(line = dwarf_onesrcline(line_buffer, i)) )
					goto out_err;
				if (dwarf_lineno(line, &line_nr) != 0)
					goto out_err;
				if (!(line_source = dwarf_linesrc(line, &mtime, &len)) )
					goto out_err;
				if (dwarf_linebeginstatement(line, &is_code) != 0)
					goto out_err;

				if (dwarf_lineaddr(line, &addr) != 0)
					goto out_err;

				if (line_nr && is_code) {
					std::string full_file_path;
					std::string file_path = line_source;

					if (!addressIsValid(addr))
						continue;

					/* Use the full compilation path unless the source already
					 * has an absolute path */
					std::string dir = src_dirs[0] == NULL ? "" : src_dirs[0];
					full_file_path = dir_concat(dir, line_source);
					if (line_source[0] != '/')
						file_path = full_file_path;

					/******** replace the path information found in the debug symbols with *********/
					/******** the value from in the newRoot variable.         *********/
					std::string rp;
					if (m_origRoot.length() > 0 && m_newRoot.length() > 0) {
 					  std::string dwarfPath = file_path;
					  size_t dwIndex = dwarfPath.find(m_origRoot);
					  if (dwIndex != std::string::npos) {
					    dwarfPath.replace(dwIndex, m_origRoot.length(), m_newRoot);
					    rp = get_real_path(dwarfPath);
					  }
					}
					else {
					  rp = get_real_path(file_path);
					}

					if (rp != "")
					{
						file_path = full_file_path = rp;
					}

					for (LineListenerList_t::const_iterator it = m_lineListeners.begin();
							it != m_lineListeners.end();
							++it)
						(*it)->onLine(file_path, line_nr, adjustAddressBySegment(addr));
				}
			}
		}

out_err:
		/* Shutdown libdwarf */
		if (dwarf_end(dbg) != 0)
			goto out_err;

		close(fd);

		return true;
	}

	bool parseOneElf()
	{
		Elf_Scn *scn = NULL;
		size_t shstrndx;
		bool ret = false;
		bool setupSegments = false;
		FileList_t gcdaFiles; // List of gcov data files scanned from .rodata
		bool doScanForGcda = IConfiguration::getInstance().keyAsInt("gcov");
		int fd;
		unsigned int i;

		fd = ::open(m_filename.c_str(), O_RDONLY, 0);
		if (fd < 0) {
				error("Cannot open %s\n", m_filename.c_str());
				return false;
		}

		if (!(m_elf = elf_begin(fd, ELF_C_READ, NULL)) ) {
				error("elf_begin failed on %s\n", m_filename.c_str());
				goto out_open;
		}

		if (elf_getshdrstrndx(m_elf, &shstrndx) < 0) {
				error("elf_getshstrndx failed on %s\n", m_filename.c_str());
				goto out_elf_begin;
		}

		setupSegments = m_curSegments.size() == 0;
		while ( (scn = elf_nextscn(m_elf, scn)) != NULL )
		{
			uint64_t sh_type;
			uint64_t sh_addr;
			uint64_t sh_size;
			uint64_t sh_flags;
			uint64_t sh_name;
			uint64_t n_namesz;
			uint64_t n_descsz;
			uint64_t n_type;
			char *n_data;
			char *name;

			if (m_elfIs32Bit) {
				Elf32_Shdr *shdr32 = elf32_getshdr(scn);

				sh_type = shdr32->sh_type;
				sh_addr = shdr32->sh_addr;
				sh_size = shdr32->sh_size;
				sh_flags = shdr32->sh_flags;
				sh_name = shdr32->sh_name;
			} else {
				Elf64_Shdr *shdr64 = elf64_getshdr(scn);

				sh_type = shdr64->sh_type;
				sh_addr = shdr64->sh_addr;
				sh_size = shdr64->sh_size;
				sh_flags = shdr64->sh_flags;
				sh_name = shdr64->sh_name;
			}

			Elf_Data *data = elf_getdata(scn, NULL);

			name = elf_strptr(m_elf, shstrndx, sh_name);
			if(!data) {
					error("elf_getdata failed on section %s in %s\n",
							name, m_filename.c_str());
					goto out_elf_begin;
			}

			// Parse rodata to find gcda files
			if (doScanForGcda && data->d_buf && strcmp(name, ".rodata") == 0) {
				const char *dataPtr = (const char *)data->d_buf;

				for (size_t i = 0; i < data->d_size - 5; i++) {
					const char *p = &dataPtr[i];

					if (memcmp(p, (const void *)"gcda\0", 5) != 0)
						continue;

					const char *gcda = p;

					// Rewind to start of string
					while (gcda != dataPtr && *gcda != '\0')
						gcda--;

					// Rewound until start of rodata?
					if (gcda == dataPtr)
						continue;

					std::string file(gcda + 1);

					gcdaFiles.push_back(file);

					// Notify listeners that we have found gcda files
					for (FileListenerList_t::const_iterator it = m_fileListeners.begin();
							it != m_fileListeners.end();
							++it)
						(*it)->onFile(File(file, IFileParser::FLG_TYPE_COVERAGE_DATA));
				}
			}

			if (sh_type == SHT_NOTE && data->d_buf) {
				if (m_elfIs32Bit) {
					Elf32_Nhdr *nhdr32 = (Elf32_Nhdr *)data->d_buf;

					n_namesz = nhdr32->n_namesz;
					n_descsz = nhdr32->n_descsz;
					n_type = nhdr32->n_type;
					n_data = (char *)data->d_buf + sizeof (Elf32_Nhdr);
				} else {
					Elf64_Nhdr *nhdr64 = (Elf64_Nhdr *)data->d_buf;

					n_namesz = nhdr64->n_namesz;
					n_descsz = nhdr64->n_descsz;
					n_type = nhdr64->n_type;
					n_data = (char *)data->d_buf + sizeof (Elf64_Nhdr);
				}

				if (::strcmp(n_data, ELF_NOTE_GNU) == 0 &&
				    n_type == NT_GNU_BUILD_ID) {
					const char *hex_digits = "0123456789abcdef";
					unsigned char *build_id;

					build_id = (unsigned char *)n_data + n_namesz;
					for (i = 0; i < n_descsz; i++) {
						m_buildId.push_back(hex_digits[(build_id[i] >> 4) & 0xf]);
						m_buildId.push_back(hex_digits[(build_id[i] >> 0) & 0xf]);
					}
				}
			}

			if ((sh_flags & (SHF_EXECINSTR | SHF_ALLOC)) != (SHF_EXECINSTR | SHF_ALLOC))
				continue;

			// If we have segments already, we can safely skip this
			if (setupSegments)
				m_curSegments.push_back(Segment(sh_addr, sh_addr, sh_size));
			m_executableSegments.push_back(Segment(sh_addr, sh_addr, sh_size));
		}

		// If we have gcda files, try to find the corresponding gcno dittos
		for (FileList_t::iterator it = gcdaFiles.begin();
				it != gcdaFiles.end();
				++it) {
			std::string &gcno = *it; // Modify in-place
			size_t sz = gcno.size();

			// .gcda -> .gcno
			gcno[sz - 2] = 'n';
			gcno[sz - 1] = 'o';

			if (file_exists(gcno))
				m_gcnoFiles.push_back(gcno);
		}

		elf_end(m_elf);
		if (!(m_elf = elf_begin(fd, ELF_C_READ, NULL)) ) {
			error("elf_begin failed on %s\n", m_filename.c_str());
			goto out_open;
		}

		ret = true;

out_elf_begin:
		elf_end(m_elf);
out_open:
		close(fd);

		return ret;
	}

	void registerLineListener(ILineListener &listener)
	{
		m_lineListeners.push_back(&listener);
	}

	void registerFileListener(IFileListener &listener)
	{
		m_fileListeners.push_back(&listener);
	}

private:
	class Segment
	{
	public:
		Segment(uint64_t paddr, uint64_t vaddr, uint64_t size) :
			m_paddr(paddr), m_vaddr(vaddr), m_size(size)
		{
		}

		uint64_t m_paddr;
		uint64_t m_vaddr;
		size_t m_size;
	};

	typedef std::vector<Segment> SegmentList_t;
	typedef std::vector<ILineListener *> LineListenerList_t;
	typedef std::vector<IFileListener *> FileListenerList_t;
	typedef std::vector<std::string> FileList_t;

	bool addressIsValid(uint64_t addr)
	{
		for (SegmentList_t::const_iterator it = m_executableSegments.begin();
				it != m_executableSegments.end();
				++it) {
			if (addr >= it->m_paddr && addr < it->m_paddr + it->m_size) {
				return true;
			}
		}

		return false;
	}

	uint64_t adjustAddressBySegment(uint64_t addr)
	{
		for (SegmentList_t::const_iterator it = m_curSegments.begin();
				it != m_curSegments.end();
				++it) {
			if (addr >= it->m_paddr && addr < it->m_paddr + it->m_size) {
				addr = (addr - it->m_paddr + it->m_vaddr);
				break;
			}
		}

		return addr;
	}

	SegmentList_t m_curSegments;
	SegmentList_t m_executableSegments;
	FileList_t m_gcnoFiles;

	struct Elf *m_elf;
	bool m_elfIs32Bit;
	LineListenerList_t m_lineListeners;
	FileListenerList_t m_fileListeners;
	std::string m_filename;
	std::string m_buildId;
	bool m_isMainFile;
	uint64_t m_checksum;
	bool m_initialized;

	/***** Add strings to update path information. *******/
	std::string m_origRoot;
	std::string m_newRoot;

	IFilter *m_filter;
};

static ElfInstance g_instance;
