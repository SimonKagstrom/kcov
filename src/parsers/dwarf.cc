#include "dwarf.hh"

#include <utils.hh>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


using namespace kcov;

DwarfParser::DwarfParser() :
		m_fd(-1),
		m_dwarf(NULL)
{
}

DwarfParser::~DwarfParser()
{
	close();
}

void DwarfParser::forEachLine(IFileParser::ILineListener& listener)
{
	if (!m_dwarf)
		return;

	Dwarf_Off offset = 0;
	Dwarf_Off lastOffset = 0;
	size_t headerSize;

	/* Iterate over the headers */
	while (dwarf_nextcu(m_dwarf, offset, &offset, &headerSize, 0, 0, 0) == 0) {
		Dwarf_Lines* lines;
		Dwarf_Files *files;
		size_t lineCount;
		size_t fileCount;
		Dwarf_Die die;
		unsigned int i;

		if (dwarf_offdie(m_dwarf, lastOffset + headerSize, &die) == NULL)
			continue;

		lastOffset = offset;

		/* Get the source lines */
		if (dwarf_getsrclines(&die, &lines, &lineCount) != 0)
			continue;

		/* And the files */
		if (dwarf_getsrcfiles(&die, &files, &fileCount) != 0)
			continue;

		const char *const *srcDirs;
		size_t ndirs = 0;

		/* Lookup the compilation path */
		if (dwarf_getsrcdirs(files, &srcDirs, &ndirs) != 0)
			continue;

		if (ndirs == 0)
			continue;

		/* Iterate through the source lines */
		for (i = 0; i < lineCount; i++) {
			Dwarf_Line *line;
			int lineNr = 0;
			const char* lineSource;
			Dwarf_Word mtime, len;
			bool isCode;
			Dwarf_Addr addr;

			if ( !(line = dwarf_onesrcline(lines, i)) )
				continue;

			if (dwarf_lineno(line, &lineNr) != 0)
				continue;

			if (!(lineSource = dwarf_linesrc(line, &mtime, &len)) )
				continue;

			if (dwarf_linebeginstatement(line, &isCode) != 0)
				continue;

			if (dwarf_lineaddr(line, &addr) != 0)
				continue;

			// Invalid line number?
			if (lineNr == 0)
				continue;

			// Non-code?
			if (!isCode)
				continue;

			/* Use the full compilation path unless the source already
			 * has an absolute path */
			std::string fullFilePath;
			std::string filePath = lineSource;

			std::string dir = srcDirs[0] == NULL ? "" : srcDirs[0];
			fullFilePath = dir_concat(dir, lineSource);
			if (lineSource[0] != '/')
				filePath = fullFilePath;

			listener.onLine(filePath, lineNr, addr);
		}
	}
}

bool DwarfParser::open(const std::string& filename)
{
	close();

	m_fd = ::open(filename.c_str(), O_RDONLY);

	if (m_fd < 0)
		return false;

	/* Initialize libdwarf from the file descriptor */
	m_dwarf = dwarf_begin(m_fd, DWARF_C_READ);

	if (!m_dwarf) {
		::close(m_fd);
		m_fd = -1;

		return false;
	}

	return true;
}

void DwarfParser::close()
{
	if (m_dwarf)
		dwarf_end(m_dwarf);

	if (m_fd >= 0)
		::close(m_fd);

	m_fd = -1;
	m_dwarf = NULL;
}
