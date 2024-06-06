namespace std
{
class type_info;
}

#include "writer-base.hh"

#include <configuration.hh>
#include <file-parser.hh>
#include <fstream>
#include <iostream>
#include <list>
#include <reporter.hh>
#include <string>
#include <unordered_map>
#include <utils.hh>
#include <writer.hh>

using namespace kcov;

class JsonWriter : public WriterBase
{
public:
    JsonWriter(IFileParser& parser, IReporter& reporter, const std::string& outFile)
        : WriterBase(parser, reporter)
        , m_outFile(outFile)
    {
    }

    void onStartup()
    {
    }

    void onStop()
    {
        if (IConfiguration::getInstance().keyAsInt("dump-summary"))
        {
            auto nTotalExecutedLines = 0u;
            auto nTotalCodeLines = 0u;
            double percentCovered = 0.0;
            auto first_file = true;

            printf("{\n  \"files\": [\n");
            for (const auto& cur : m_files)
            {
                auto& file = cur.second;
                unsigned int nExecutedLines = 0;
                unsigned int nCodeLines = 0;

                for (unsigned int n = 1; n < file->m_lastLineNr; n++)
                {
                    IReporter::LineExecutionCount cnt =
                        m_reporter.getLineExecutionCount(file->m_name, n);
                    if (m_reporter.lineIsCode(file->m_name, n))
                    {
                        nExecutedLines += !!cnt.m_hits;
                        nCodeLines++;
                        nTotalExecutedLines += !!cnt.m_hits;
                        nTotalCodeLines++;
                    }
                }
                if (nCodeLines > 0)
                {
                    percentCovered = static_cast<double>(nExecutedLines) / nCodeLines * 100;
                }

                if (!first_file)
                {
                    printf(",\n");
                }

                printf("%s",
                       fmt("    {"
                           "\"file\": \"%s\", "
                           "\"percent_covered\": \"%.2f\"}",
                           file->m_name.c_str(),
                           percentCovered)
                           .c_str());

                first_file = false;
            }

            percentCovered = 0;
            if (nTotalCodeLines > 0)
            {
                percentCovered = static_cast<double>(nTotalExecutedLines) / nTotalCodeLines * 100;
            }

            printf("%s",
                   fmt("\n"
                       "  ],\n"
                       "  \"percent_covered\": \"%.2f\",\n"
                       "  \"covered_lines\": %d,\n"
                       "  \"total_lines\": %d\n"
                       "}\n",
                       percentCovered,
                       nTotalExecutedLines,
                       nTotalCodeLines)
                       .c_str());
        }
    }

    void write()
    {
        std::ofstream out(m_outFile);

        // Output directory not writable?
        if (!out.is_open())
            return;

        out << "{\n";
        out << "  \"files\": [\n";

        IConfiguration& conf = IConfiguration::getInstance();
        unsigned int nTotalExecutedLines = 0;
        unsigned int nTotalCodeLines = 0;
        double percentCovered = 0.0;
        bool firstFile = true;

        for (FileMap_t::const_iterator it = m_files.begin(); it != m_files.end(); ++it)
        {
            File* file = it->second;
            unsigned int nExecutedLines = 0;
            unsigned int nCodeLines = 0;

            for (unsigned int n = 1; n < file->m_lastLineNr; n++)
            {
                IReporter::LineExecutionCount cnt =
                    m_reporter.getLineExecutionCount(file->m_name, n);
                if (m_reporter.lineIsCode(file->m_name, n))
                {
                    nExecutedLines += !!cnt.m_hits;
                    nCodeLines++;
                    nTotalExecutedLines += !!cnt.m_hits;
                    nTotalCodeLines++;
                }
            }
            if (nCodeLines > 0)
                percentCovered = static_cast<double>(nExecutedLines) / nCodeLines * 100;

            if (firstFile)
                firstFile = false;
            else
                out << ",\n";

            out << fmt("    {"
                       "\"file\": \"%s\", "
                       "\"percent_covered\": \"%.2f\", "
                       "\"covered_lines\": \"%d\", "
                       "\"total_lines\": \"%d\""
                       "}",
                       file->m_name.c_str(),
                       percentCovered,
                       nExecutedLines,
                       nCodeLines);
        }

        percentCovered = 0;
        if (nTotalCodeLines > 0)
            percentCovered = static_cast<double>(nTotalExecutedLines) / nTotalCodeLines * 100;

        out << fmt("\n"
                   "  ],\n"
                   "  \"percent_covered\": \"%.2f\",\n"
                   "  \"covered_lines\": %d,\n"
                   "  \"total_lines\": %d,\n"
                   "  \"percent_low\": %d,\n"
                   "  \"percent_high\": %d,\n"
                   "  \"command\": \"%s\",\n"
                   "  \"date\": \"%s\"\n"
                   "}\n",
                   percentCovered,
                   nTotalExecutedLines,
                   nTotalCodeLines,
                   conf.keyAsInt("low-limit"),
                   conf.keyAsInt("high-limit"),
                   escape_json(conf.keyAsString("command-name")).c_str(),
                   getDateNow().c_str());
    }

private:
    std::string getDateNow()
    {
        time_t t;
        struct tm* tm;
        char date_buf[128];

        t = time(NULL);
        tm = localtime(&t);
        strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", tm);

        return std::string(date_buf);
    }

    std::string m_outFile;
};

namespace kcov
{
IWriter&
createJsonWriter(IFileParser& parser, IReporter& reporter, const std::string& outFile)
{
    return *new JsonWriter(parser, reporter, outFile);
}
} // namespace kcov
