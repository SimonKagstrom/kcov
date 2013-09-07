#include "writer-base.hh"
#include <utils.hh>

#include <stdio.h>

using namespace kcov;

struct summaryStruct
{
	uint32_t nLines;
	uint32_t nExecutedLines;
	char name[256];
};

int WriterBase::File::fileNameCount;

WriterBase::WriterBase(IElf &elf, IReporter &reporter, IOutputHandler &output) :
		m_elf(elf), m_reporter(reporter)
{
		m_commonPath = "not set";
		m_elf.registerLineListener(*this);
}

WriterBase::File::File(const char *filename) :
						m_name(filename), m_codeLines(0), m_executedLines(0), m_lastLineNr(0)
{
	size_t pos = m_name.rfind('/');

	if (pos != std::string::npos)
		m_fileName = m_name.substr(pos + 1, std::string::npos);
	else
		m_fileName = m_name;

	// Make this name unique (we might have several files with the same name)
	m_outFileName = fmt("%s.%d.html", m_fileName.c_str(), fileNameCount);
	fileNameCount++;

	readFile(filename);
}

void WriterBase::File::readFile(const char *filename)
{
	FILE *fp = fopen(filename, "r");
	unsigned int lineNr = 1;

	panic_if(!fp, "Can't open %s", filename);

	while (1)
	{
		char *lineptr = NULL;
		ssize_t res;
		size_t n;

		res = getline(&lineptr, &n, fp);
		if (res < 0)
			break;
		m_lineMap[lineNr] = std::string(lineptr);

		free((void *)lineptr);
		lineNr++;
	}

	m_lastLineNr = lineNr;

	fclose(fp);
}


void WriterBase::onLine(const char *file, unsigned int lineNr, unsigned long addr)
{
	if (m_files.find(std::string(file)) != m_files.end())
		return;

	if (m_nonExistingFiles.find(std::string(file)) != m_nonExistingFiles.end())
		return;

	if (!file_exists(file)) {
		m_nonExistingFiles[std::string(file)] = NULL;
		return;
	}

	m_files[std::string(file)] = new File(file);
}


void *WriterBase::marshalSummary(IReporter::ExecutionSummary &summary,
		std::string &name, size_t *sz)
{
	struct summaryStruct *p;

	p = (struct summaryStruct *)xmalloc(sizeof(struct summaryStruct));
	memset(p, 0, sizeof(*p));

	p->nLines = summary.m_lines;
	p->nExecutedLines = summary.m_executedLines;
	strncpy(p->name, name.c_str(), sizeof(p->name) - 1);

	*sz = sizeof(*p);

	return (void *)p;
}

bool WriterBase::unMarshalSummary(void *data, size_t sz,
		IReporter::ExecutionSummary &summary,
		std::string &name)
{
	struct summaryStruct *p = (struct summaryStruct *)data;

	if (sz != sizeof(*p))
		return false;

	summary.m_lines = p->nLines;
	summary.m_executedLines = p->nExecutedLines;
	name = std::string(p->name);

	return true;
}

void WriterBase::setupCommonPaths()
{
	for (FileMap_t::iterator it = m_files.begin();
			it != m_files.end();
			it++) {
		File *file = it->second;

		if (m_commonPath == "not set")
			m_commonPath = file->m_name;

		/* Already matching? */
		if (file->m_name.find(m_commonPath) == 0)
			continue;

		while (1) {
			size_t pos = m_commonPath.rfind('/');
			if (pos == std::string::npos)
				break;

			m_commonPath = m_commonPath.substr(0, pos);
			if (file->m_name.find(m_commonPath) == 0)
				break;
		}
	}
}

void WriterBase::stop()
{
}
