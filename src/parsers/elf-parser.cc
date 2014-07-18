#include <file-parser.hh>
#include <engine.hh>
#include <utils.hh>
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
		m_elf = nullptr;
		m_filename = "";
		m_checksum = 0;
		m_elfIs32Bit = true;
		m_isMainFile = true;
		m_initialized = false;

		IParserManager::getInstance().registerParser(*this);
	}

	virtual ~ElfInstance()
	{
	}

	uint64_t getChecksum()
	{
		return m_checksum;
	}

	bool elfIs64Bit()
	{
		return !m_elfIs32Bit;
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
			m_origRoot = IConfiguration::getInstance().getOriginalPathPrefix();
			m_newRoot  = IConfiguration::getInstance().getNewPathPrefix();

			m_fixedAddresses = IConfiguration::getInstance().getFixedBreakpoints();

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

		if (!(elf = elf_begin(fd, ELF_C_READ, nullptr)) ) {
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
				m_elfIs32Bit = raw[EI_CLASS] == ELFCLASS32;

				if (elfIs64Bit() != machine_is_64bit())
					IConfiguration::getInstance().setParseSolibs(false);
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
		parseOneDwarf();

		for (FileListenerList_t::const_iterator it = m_fileListeners.begin();
				it != m_fileListeners.end();
				++it)
			(*it)->onFile(m_filename.c_str(), m_isMainFile ? IFileParser::FLG_NONE : IFileParser::FLG_TYPE_SOLIB);

		// After the first, all other are solibs
		m_isMainFile = false;

		// One-time setup of fixed breakpoints
		for (FixedAddressList_t::const_iterator it = m_fixedAddresses.begin();
				it != m_fixedAddresses.end();
				++it) {
			for (LineListenerList_t::const_iterator itL = m_lineListeners.begin();
					itL != m_lineListeners.end();
					++itL)
				(*itL)->onLine("", 0, *it);
		}
		m_fixedAddresses.clear();

		return true;
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
				if (m_isMainFile)
					error("Cannot open %s\n", debug_file.c_str());
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

			if (dwarf_offdie(dbg, last_offset + hdr_size, &die) == nullptr)
				goto out_err;

			last_offset = offset;

			/* Get the source lines */
			if (dwarf_getsrclines(&die, &line_buffer, &line_count) != 0)
				continue;

			/* And the files */
			if (dwarf_getsrcfiles(&die, &file_buffer, &file_count) != 0)
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
					const char *const *src_dirs;
					std::string full_file_path;
					std::string file_path = line_source;
					size_t ndirs = 0;

					/* Lookup the compilation path */
					if (dwarf_getsrcdirs(file_buffer, &src_dirs, &ndirs) != 0)
						continue;

					if (ndirs == 0)
						continue;

					if (!addressIsValid(addr))
						continue;

					/* Use the full compilation path unless the source already
					 * has an absolute path */
					std::string dir = src_dirs[0] == nullptr ? "" : src_dirs[0];
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
						(*it)->onLine(file_path.c_str(), line_nr, adjustAddressBySegment(addr));
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
		Elf_Scn *scn = nullptr;
		size_t shstrndx;
		bool ret = false;
		bool setupSegments = false;
		int fd;
		unsigned int i;

		fd = ::open(m_filename.c_str(), O_RDONLY, 0);
		if (fd < 0) {
				error("Cannot open %s\n", m_filename.c_str());
				return false;
		}

		if (!(m_elf = elf_begin(fd, ELF_C_READ, nullptr)) ) {
				error("elf_begin failed on %s\n", m_filename.c_str());
				goto out_open;
		}

		if (elf_getshdrstrndx(m_elf, &shstrndx) < 0) {
				error("elf_getshstrndx failed on %s\n", m_filename.c_str());
				goto out_elf_begin;
		}

		setupSegments = m_curSegments.size() == 0;
		while ( (scn = elf_nextscn(m_elf, scn)) != nullptr )
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

			Elf_Data *data = elf_getdata(scn, nullptr);

			name = elf_strptr(m_elf, shstrndx, sh_name);
			if(!data) {
					error("elf_getdata failed on section %s in %s\n",
							name, m_filename.c_str());
					goto out_elf_begin;
			}

			if (sh_type == SHT_NOTE) {
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
		elf_end(m_elf);
		if (!(m_elf = elf_begin(fd, ELF_C_READ, nullptr)) ) {
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
	typedef std::list<uint64_t> FixedAddressList_t;

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

	struct Elf *m_elf;
	bool m_elfIs32Bit;
	LineListenerList_t m_lineListeners;
	FileListenerList_t m_fileListeners;
	std::string m_filename;
	std::string m_buildId;
	bool m_isMainFile;
	uint64_t m_checksum;
	bool m_initialized;
	FixedAddressList_t m_fixedAddresses;

	/***** Add strings to update path information. *******/
	std::string m_origRoot;
	std::string m_newRoot;
};

static ElfInstance g_instance;
