#include <elf.hh>
#include <engine.hh>
#include <utils.hh>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libelf.h>
#include <dwarf.h>
#include <elfutils/libdw.h>
#include <map>
#include <list>
#include <string>

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

class Elf : public IElf
{
public:
	Elf()
	{
		m_elf = NULL;
		m_filename = NULL;
		m_checksum = 0;
	}
	virtual ~Elf()
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

	bool addFile(const char *filename)
	{
		free((void *)m_filename);
		m_filename = strdup(filename);

		return checkFile();
	}

	bool checkFile()
	{
		Elf *elf;
		bool out = true;
		int fd;

		fd = ::open(m_filename, O_RDONLY, 0);
		if (fd < 0) {
				error("Cannot open %s\n", m_filename);
				return false;
		}

		if (!(elf = elf_begin(fd, ELF_C_READ, NULL)) ) {
				error("elf_begin failed on %s\n", m_filename);
				out = false;
				goto out_open;
		}
		if (!elf32_getehdr(elf)) {
				error("elf32_getehdr failed on %s\n", m_filename);
				out = false;
		}
		elf_end(elf);

out_open:
		close(fd);

		return out;
	}

	static int phdrCallback(struct dl_phdr_info *info, size_t size,
			void *data)
	{
		Elf *p = (Elf *)data;

		p->handlePhdr(info, size);

		return 0;
	}

	void handlePhdr(struct dl_phdr_info *info, size_t size)
	{
		int phdr;

		if (strlen(info->dlpi_name) != 0) {
			free( (void *)m_filename );
			m_filename = strdup(info->dlpi_name);
		}
		kcov_debug(ELF_MSG, "ELF parsing %s\n", m_filename);

		m_curSegments.clear();
		for (phdr = 0; phdr < info->dlpi_phnum; phdr++) {
			const ElfW(Phdr) *cur = &info->dlpi_phdr[phdr];

			if (cur->p_type != PT_LOAD)
				continue;

			kcov_debug(ELF_MSG, "ELF seg 0x%08x -> 0x%08x...0x%08x\n",
					cur->p_paddr, info->dlpi_addr + cur->p_vaddr,
					info->dlpi_addr + cur->p_vaddr + cur->p_memsz);
			m_curSegments.push_back(Segment(cur->p_paddr, info->dlpi_addr + cur->p_vaddr,
					cur->p_memsz, cur->p_align));
		}

		parseOneElf();
		parseOneDwarf();
	}

	bool parse()
	{
//		dl_iterate_phdr(phdrCallback, (void *)this);
		struct stat st;

		if (lstat(m_filename, &st) < 0)
			return 0;

		m_checksum = ((uint64_t)st.st_mtim.tv_sec << 32) | ((uint64_t)st.st_mtim.tv_nsec);

		parseOneElf();
		parseOneDwarf();

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

			if (dwarf_offdie(dbg, last_offset + hdr_size, &die) == NULL)
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

					if (addr < 0x1000)
					{
						// FIXME! This is not a correct check
						continue;
					}

					/* Use the full compilation path unless the source already
					 * has an absolute path */
					full_file_path = dir_concat(src_dirs[0], line_source);
					if (line_source[0] != '/')
						file_path = full_file_path;

					char *rp = ::realpath(file_path, NULL);

					if (rp)
					{
						free((void *)full_file_path);
						file_path = full_file_path = rp;
					}

					for (ListenerList_t::iterator it = m_listeners.begin();
							it != m_listeners.end();
							it++)
						(*it)->onLine(file_path, line_nr, (unsigned long)addr);

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
		Elf_Scn *scn = NULL;
		Elf32_Ehdr *ehdr;
		size_t shstrndx;
		bool ret = false;
		int fd;

		fd = ::open(m_filename, O_RDONLY, 0);
		if (fd < 0) {
				error("Cannot open %s\n", m_filename);
				return false;
		}

		if (!(m_elf = elf_begin(fd, ELF_C_READ, NULL)) ) {
				error("elf_begin failed on %s\n", m_filename);
				goto out_open;
		}


		if (!(ehdr = elf32_getehdr(m_elf))) {
				error("elf32_getehdr failed on %s\n", m_filename);
				goto out_elf_begin;
		}

		if (elf_getshdrstrndx(m_elf, &shstrndx) < 0) {
				error("elf_getshstrndx failed on %s\n", m_filename);
				goto out_elf_begin;
		}

		while ( (scn = elf_nextscn(m_elf, scn)) != NULL )
		{
			Elf32_Shdr *shdr = elf32_getshdr(scn);
			Elf_Data *data = elf_getdata(scn, NULL);
			char *name;

			name = elf_strptr(m_elf, shstrndx, shdr->sh_name);
			if(!data) {
					error("elf_getdata failed on section %s in %s\n",
							name, m_filename);
					goto out_elf_begin;
			}

			/* Handle symbols */
			if (shdr->sh_type == SHT_SYMTAB)
				handleSymtab(scn);
			if (shdr->sh_type == SHT_DYNSYM)
				handleDynsym(scn);
		}
		elf_end(m_elf);
		if (!(m_elf = elf_begin(fd, ELF_C_READ, NULL)) ) {
			error("elf_begin failed on %s\n", m_filename);
			goto out_open;
		}
		while ( (scn = elf_nextscn(m_elf, scn)) != NULL )
		{
			Elf32_Shdr *shdr = elf32_getshdr(scn);
			char *name = elf_strptr(m_elf, shstrndx, shdr->sh_name);

			// .rel.plt
			if (shdr->sh_type == SHT_REL && strcmp(name, ".rel.plt") == 0)
				handleRelPlt(scn);
		}

		ret = true;

out_elf_begin:
		elf_end(m_elf);
out_open:
		close(fd);

		return ret;
	}

	void registerListener(IListener &listener)
	{
		m_listeners.push_back(&listener);
	}

private:
	class Segment
	{
	public:
		Segment(ElfW(Addr) paddr, ElfW(Addr) vaddr, size_t size, ElfW(Word) align) :
			m_paddr(paddr), m_vaddr(vaddr), m_align(align), m_size(size)
		{
		}

		ElfW(Addr) m_paddr;
		ElfW(Addr) m_vaddr;
		ElfW(Word) m_align;
		size_t m_size;
	};

	typedef std::list<Segment> SegmentList_t;
	typedef std::list<IListener *> ListenerList_t;

	void *offsetTableToAddress(Elf32_Addr addr)
	{
		/*
		 * The .got.plt table contains a pointer to the push instruction
		 * below:
		 *
		 *  08070f10 <pthread_self@plt>:
		 *   8070f10:       ff 25 58 93 0b 08       jmp    *0x80b9358
		 *   8070f16:       68 b0 06 00 00          push   $0x6b0
		 *
		 * so to get the entry point, we rewind the pointer to the start
		 * of the jmp.
		 */
		return (void *)(addr - 6);
	}

	ElfW(Addr) adjustAddressBySegment(ElfW(Addr) addr)
	{
		for (SegmentList_t::iterator it = m_curSegments.begin();
				it != m_curSegments.end(); it++) {
			Segment cur = *it;

			if (addr >= cur.m_paddr && addr < cur.m_paddr + cur.m_size) {
				addr = (addr - cur.m_paddr + cur.m_vaddr);
				break;
			}
		}

		return addr;
	}

	void handleRelPlt(Elf_Scn *scn)
	{
		Elf_Data *data = elf_getdata(scn, NULL);
		Elf32_Rel *r = (Elf32_Rel *)data->d_buf;
		int n = data->d_size / sizeof(Elf32_Rel);

		panic_if(n <= 0,
				"Section data too small (%zd) - no symbols\n",
				data->d_size);

		for (int i = 0; i < n; i++, r++) {
			adjustAddressBySegment(r->r_offset);

			// FIXME! Do something here
		}
	}

	void handleDynsym(Elf_Scn *scn)
	{
		handleSymtabGeneric(scn, SYM_DYNAMIC);
	}

	void handleSymtab(Elf_Scn *scn)
	{
		handleSymtabGeneric(scn, SYM_NORMAL);
	}

	void handleSymtabGeneric(Elf_Scn *scn, enum SymbolType symType)
	{
		Elf_Data *data = elf_getdata(scn, NULL);
		Elf32_Sym *s = (Elf32_Sym *)data->d_buf;
		int n = data->d_size / sizeof(Elf32_Sym);

		panic_if(n <= 0,
				"Section data too small (%zd) - no symbols\n",
				data->d_size);

		/* Iterate through all symbols */
		for (int i = 0; i < n; i++)
		{
			//const char *sym_name = elf_strptr(m_elf, shdr->sh_link, s->st_name);
			int type = ELF32_ST_TYPE(s->st_info);

			/* Ohh... This is an interesting symbol, add it! */
			if ( type == STT_FUNC) {
				adjustAddressBySegment(s->st_value);
				// s->st_size;
			}

			s++;
		}
	}

	SegmentList_t m_curSegments;

	Elf *m_elf;
	ListenerList_t m_listeners;
	IListener *m_listener;
	const char *m_filename;
	uint64_t m_checksum;
};

IElf *IElf::open(const char *filename)
{
	static bool initialized = false;
	Elf *p;

	if (!initialized) {
		panic_if(elf_version(EV_CURRENT) == EV_NONE,
				"ELF version failed\n");
		initialized = true;
	}

	p = new Elf();

	if (p->addFile(filename) == false) {
		delete p;

		return NULL;
	}

	return p;
}
