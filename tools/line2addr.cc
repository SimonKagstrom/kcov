#include <configuration.hh>
#include <file-parser.hh>
#include <utils.hh>

using namespace kcov;

class Listener : public IFileParser::ILineListener
{
public:
	Listener(IFileParser &parser, const std::string &filePattern, int lineNr) :
		m_filePattern(filePattern),
		m_lineNr(lineNr)
	{
		parser.registerLineListener(*this);
	}

	virtual ~Listener()
	{
	}

	void onLine(const std::string &file, unsigned int lineNr, uint64_t addr)
	{
		if (file.find(m_filePattern) != std::string::npos) {
			if (m_lineNr < 0 || lineNr == (unsigned int)m_lineNr)
				report(addr);
		}
	}

private:
	void report(unsigned long addr)
	{
		printf("0x%lx\n", addr);
	}

	const std::string m_filePattern;
	int m_lineNr;
};

int main(int argc, const char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: line2addr in-file file-pattern [line-nr]\n");
		return 1;
	}

	std::string file(argv[1]);
	std::string fileName(argv[2]);
	int lineNr = -1;

	if (argc >= 4) {
		if (!string_is_integer(argv[3])) {
			fprintf(stderr, "line-nr argument (%s) must be integer\n", argv[3]);
			exit(1);
		}

		lineNr = string_to_integer(argv[3]);
	}


	IFileParser *parser = IParserManager::getInstance().matchParser(file);
	if (!parser) {
		fprintf(stderr, "Can't match parser for %s\n", file.c_str());
		return 1;
	}

	Listener listener(*parser, fileName, lineNr);

	parser->addFile(file);

	return parser->parse() ? 0 : 1;
}
