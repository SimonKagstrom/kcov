#include "writer-base.hh"
#include <utils.hh>

#include <swap-endian.hh>

#include <stdio.h>

using namespace kcov;

#define SUMMARY_MAGIC   0x456d696c
#define SUMMARY_VERSION 2

struct summaryStruct
{
	uint32_t magic;
	uint32_t version;
	uint32_t includeInTotals;
	uint32_t nLines;
	uint32_t nExecutedLines;
	char name[256];
};

int WriterBase::File::fileNameCount;

WriterBase::WriterBase(IFileParser &parser, IReporter &reporter) :
		m_fileParser(parser), m_reporter(reporter),
		m_commonPath("not set"),
		m_filter(IFilter::getInstance())
{
		m_fileParser.registerLineListener(*this);
}

WriterBase::File::File(const std::string &filename) :
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

void WriterBase::File::readFile(const std::string &filename)
{
	FILE *fp = fopen(filename.c_str(), "r");
	unsigned int lineNr = 1;

	panic_if(!fp, "Can't open %s", filename.c_str());

	while (1)
	{
		char *lineptr = nullptr;
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


void WriterBase::onLine(const std::string &file, unsigned int lineNr, unsigned long addr)
{
	if (!m_filter.runFilters(file))
		return;


	if (m_files.find(file) != m_files.end())
		return;

	if (!file_exists(file.c_str()))
		return;

	m_files[file] = new File(file);
}


void *WriterBase::marshalSummary(IReporter::ExecutionSummary &summary,
		const std::string &name, size_t *sz)
{
	struct summaryStruct *p;

	p = (struct summaryStruct *)xmalloc(sizeof(struct summaryStruct));
	memset(p, 0, sizeof(*p));

	p->magic = to_be<uint32_t>(SUMMARY_MAGIC);
	p->version = to_be<uint32_t>(SUMMARY_VERSION);
	p->includeInTotals = to_be<uint32_t>(summary.m_includeInTotals);
	p->nLines = to_be<uint32_t>(summary.m_lines);
	p->nExecutedLines = to_be<uint32_t>(summary.m_executedLines);
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

	if (be_to_host<uint32_t>(p->magic) != SUMMARY_MAGIC)
		return false;

	if (be_to_host<uint32_t>(p->version) != SUMMARY_VERSION)
		return false;

	summary.m_lines = be_to_host<uint32_t>(p->nLines);
	summary.m_executedLines = be_to_host<uint32_t>(p->nExecutedLines);
	summary.m_includeInTotals = be_to_host<uint32_t>(p->includeInTotals);
	name = std::string(p->name);

	return true;
}

void WriterBase::setupCommonPaths()
{
	for (const auto & it : m_files) {
		File *file = it.second;

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
