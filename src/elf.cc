#include <elf.hh>
#include <engine.hh>
#include <utils.hh>
#include <filter.hh>
#include <phdr_data.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libelf.h>
#include <dwarf.h>
#include <elfutils/libdw.h>
#include <map>
#include <list>
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

class ElfInstance : public IElf
{
public:
	ElfInstance() : m_filter(IFilter::getInstance())
	{
		m_elf = nullptr;
		m_filename = nullptr;
		m_checksum = 0;
		m_elfIs32Bit = true;
		m_isMainFile = true;
		/******* Swap debug source root with runtime source root *****/
		origRoot = IConfiguration::getInstance().getOriginalPathPrefix();
		newRoot  = IConfiguration::getInstance().getNewPathPrefix();
		
	}
	virtual ~ElfInstance()
	{
		free((void *)m_filename);
	}

	const char *getFilename()
	{
		return m_filename;
	}

	uint64_t getChecksum()
	{
		return m_checksum;
	}

	bool elfIs64Bit()
	{
		return !m_elfIs32Bit;
	}

	bool addFile(const char *filename, struct phdr_data_entry *data = 0)
	{
		free((void *)m_filename);
		m_filename = strdup(filename);

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

		fd = ::open(m_filename, O_RDONLY, 0);
		if (fd < 0) {
				error("Cannot open %s\n", m_filename);
				return false;
		}

		if (!(elf = elf_begin(fd, ELF_C_READ, nullptr)) ) {
				error("elf_begin failed on %s\n", m_filename);
				out = false;
				goto out_open;
		}
		if (elf_kind(elf) == ELF_K_NONE)
			out = false;

		if (m_isMainFile) {
			char *raw;
			size_t sz;

			raw = elf_getident(elf, &sz);

			if (raw && sz > EI_CLASS)
				m_elfIs32Bit = raw[EI_CLASS] == ELFCLASS32;

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

		if (lstat(m_filename, &st) < 0)
			return 0;

		parseOneElf();
		parseOneDwarf();

		for (const auto &it : m_fileListeners)
			it->onFile(m_filename, !m_isMainFile);

		// After the first, all other are solibs
		m_isMainFile = false;

		return true;
	}

	bool parseOneDwarf()
	{
		Dwarf_Off offset = 0;
		Dwarf_Off last_offset = 0;
		size_t hdr_size;
		Dwarf *dbg;
		int fd;

		fd = ::open(m_filename, O_RDONLY, 0);
		if (fd < 0) {
				error("Cannot open %s\n", m_filename);
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
			kcov_debug(ELF_MSG, "No debug symbols in %s.\n", m_filename);
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
					const char *full_file_path;
					const char *file_path = line_source;
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
					full_file_path = dir_concat(src_dirs[0], line_source);
					if (line_source[0] != '/')
						file_path = full_file_path;

					/******** replace the path information found in the debug symbols with *********/
					/******** the value from in the newRoot variable.         *********/
					char *rp = 0;
					if (origRoot.length() > 0 && newRoot.length() > 0) {
 					  std::string dwarfPath = file_path;
					  size_t dwIndex = dwarfPath.find(origRoot);
					  if (dwIndex != std::string::npos) {
					    dwarfPath.replace(dwIndex, origRoot.length(), newRoot);
					    rp = ::realpath(dwarfPath.c_str(), nullptr);

					  }
					}
					else {
					  rp = ::realpath(file_path, nullptr);
					}
					if (rp)
					{
						free((void *)full_file_path);
						file_path = full_file_path = rp;

					}

					if (m_filter.runFilters(file_path) == true)
					{
						for (const auto &it : m_lineListeners)
							it->onLine(file_path, line_nr, adjustAddressBySegment(addr));
					}

					free((void *)full_file_path);
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

		fd = ::open(m_filename, O_RDONLY, 0);
		if (fd < 0) {
				error("Cannot open %s\n", m_filename);
				return false;
		}

		if (!(m_elf = elf_begin(fd, ELF_C_READ, nullptr)) ) {
				error("elf_begin failed on %s\n", m_filename);
				goto out_open;
		}

		if (elf_getshdrstrndx(m_elf, &shstrndx) < 0) {
				error("elf_getshstrndx failed on %s\n", m_filename);
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
							name, m_filename);
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
			error("elf_begin failed on %s\n", m_filename);
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

	typedef std::list<Segment> SegmentList_t;
	typedef std::list<ILineListener *> LineListenerList_t;
	typedef std::list<IFileListener *> FileListenerList_t;

	bool addressIsValid(uint64_t addr)
	{
		for (const auto &it : m_executableSegments) {
			if (addr >= it.m_paddr && addr < it.m_paddr + it.m_size) {
				return true;
			}
		}

		return false;
	}

	uint64_t adjustAddressBySegment(uint64_t addr)
	{
		for (const auto &it : m_curSegments) {
			if (addr >= it.m_paddr && addr < it.m_paddr + it.m_size) {
				addr = (addr - it.m_paddr + it.m_vaddr);
				break;
			}
		}

		return addr;
	}

	SegmentList_t m_curSegments;
	SegmentList_t m_executableSegments;
	IFilter &m_filter;

	struct Elf *m_elf;
	bool m_elfIs32Bit;
	LineListenerList_t m_lineListeners;
	FileListenerList_t m_fileListeners;
	const char *m_filename;
	std::string m_buildId;
	bool m_isMainFile;
	uint64_t m_checksum;
        /***** Add strings to update path information. *******/
	std::string origRoot;
	std::string newRoot;

};

static ElfInstance *g_instance;
IElf *IElf::open(const char *filename)
{
	static bool initialized = false;

	if (!initialized) {
		panic_if(elf_version(EV_CURRENT) == EV_NONE,
				"ELF version failed\n");
		initialized = true;
	}

	g_instance = new ElfInstance();

	if (g_instance->addFile(filename) == false) {
		delete g_instance;

		g_instance = nullptr;

		return nullptr;
	}

	return g_instance;
}

IElf &IElf::getInstance()
{
	panic_if (!g_instance,
			"ELF object not yet created");

	return *g_instance;
}
