/**
 * @file
 * @brief A parser for OSX Mach-O dSYM files
 *
 * Based on https://github.com/facebookarchive/atosl
 */
#include <capabilities.hh>
#include <configuration.hh>
#include <dwarf.h>
#include <fcntl.h>
#include <file-parser.hh>
#include <filter.hh>
#include <iostream>
#include <libdwarf.h>
#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <map>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utils.hh>
#include <vector>

using namespace kcov;

namespace
{
static void dwarf_error_handler(Dwarf_Error error, Dwarf_Ptr userData)
{
    char *msg = dwarf_errmsg(error);
    printf("Dwarf ERROR: %s\n", msg);
}
class MachoParser : public IFileParser
{
public:
    MachoParser()
    {
        IParserManager::getInstance().registerParser(*this);
    }

    ~MachoParser()
    {
        free(m_fileData);
    }

private:
    bool addFile(const std::string& filename, struct phdr_data_entry* phdr_data) final
    {
        m_filename = filename;

        for (const auto l : m_fileListeners)
        {
            l->onFile(File(m_filename, IFileParser::FLG_NONE));
        }

        return true;
    }

    bool setMainFileRelocation(unsigned long relocation) final
    {
        return true;
    }

    uint64_t getChecksum() final
    {
        return 0;
    }

    void registerLineListener(ILineListener& listener) final
    {
        m_lineListeners.push_back(&listener);
    }

    void registerFileListener(IFileListener& listener) final
    {
        m_fileListeners.push_back(&listener);
    }

    bool parse() final
    {
        auto& conf = IConfiguration::getInstance();
        auto name = fmt("%s/%s.dSYM/Contents/Resources/DWARF/%s",
                        conf.keyAsString("binary-path").c_str(),
                        conf.keyAsString("binary-name").c_str(),
                        conf.keyAsString("binary-name").c_str());
        if (conf.keyAsInt("is-go-binary"))
        {
            name = fmt("%s/%s",
                       conf.keyAsString("binary-path").c_str(),
                       conf.keyAsString("binary-name").c_str());
        }

        m_fileData = static_cast<uint8_t*>(read_file(&m_fileSize, "%s", name.c_str()));
        if (!m_fileData)
        {
            return false;
        }

        m_readPtr = m_fileData;
        auto hdr = reinterpret_cast<mach_header_64*>(m_fileData);
        if (hdr->magic == FAT_MAGIC)
        {
            error("FAT binaries are not supported, yet");
            return false;
        }

        if (hdr->filetype != MH_DSYM && hdr->filetype != MH_EXECUTE)
        {
            error("Not a debug file");
            return false;
        }

        // Parse Mach-O
        m_readPtr += sizeof(mach_header_64);
        for (auto i = 0; i < hdr->ncmds; i++)
        {
            auto cmd = reinterpret_cast<load_command*>(m_readPtr);

            switch (cmd->cmd)
            {
            case LC_SEGMENT_64:
                commandParseSegment64();
                break;
            default:
                break;
            }

            m_readPtr += cmd->cmdsize;
        }

        // Parse DWARF

        Dwarf_Debug dbg;
        Dwarf_Error err;

        constexpr auto kPathLen = 400;
        char pathbuf[kPathLen];
        auto ret = dwarf_init_path(name.c_str(),pathbuf,kPathLen,DW_GROUPNUMBER_ANY, dwarf_error_handler, 0, &dbg,&err);
        if (ret != DW_DLV_OK)
        {
            error("Failed to init DWARF: %s", dwarf_errmsg(err));
            return false;
        }


        Dwarf_Off cu_die_offset = 0;
        Dwarf_Die cu_die = nullptr;

        while (dwarf_next_cu_header_d(dbg,
                                      1,
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      &cu_die_offset,
                                      nullptr,
                                      nullptr) == DW_DLV_OK)
        {
            Dwarf_Die no_die = nullptr;

            if (dwarf_siblingof_b(dbg, no_die, 1, &cu_die, &err) != DW_DLV_OK)
            {
                continue;
            }

            Dwarf_Small table_count = 0;
            Dwarf_Line_Context line_context = NULL;
            Dwarf_Unsigned version = 0;

            ret = dwarf_srclines_b(cu_die, &version, &table_count, &line_context, &err);
            if (ret != DW_DLV_OK)
            {
                printf("ERROR: %s\n", dwarf_errmsg(err));
                continue;
            }

            Dwarf_Line* linebuf = NULL;
            Dwarf_Signed linecount = 0;

            ret = dwarf_srclines_from_linecontext(line_context, &linebuf, &linecount, &err);
            if (ret != DW_DLV_OK)
            {
                continue;
            }

            for (auto i = 0u; i < linecount; i++)
            {
                Dwarf_Line line = linebuf[i];

                char* filename;
                Dwarf_Unsigned lineno;
                char* diename;
                Dwarf_Bool is_code = false;
                Dwarf_Addr addr = 0;

                ret = dwarf_linesrc(line, &filename, &err);
                if (ret != DW_DLV_OK)
                {
                    continue;
                }

                ret = dwarf_lineno(line, &lineno, &err);
                if (ret != DW_DLV_OK)
                {
                    continue;
                }

                if (dwarf_linebeginstatement(line, &is_code, 0) != DW_DLV_OK)
                {
                    continue;
                }


                ret = dwarf_diename(cu_die, &diename, &err);
                if (ret != DW_DLV_OK)
                {
                    continue;
                }

                if (dwarf_lineaddr(line, &addr, 0) != DW_DLV_OK)
                {
                    continue;
                }

                Dwarf_Bool is_end_seq = false;

                dwarf_lineendsequence(line, &is_end_seq, &err);
                /* End line table operations */

                if (lineno && is_code && is_end_seq == false)
                {
                    for (auto& listener : m_lineListeners)
                    {
                        listener->onLine(filename, lineno, addr);
                    }
                }

                dwarf_dealloc(dbg, diename, DW_DLA_STRING);
                dwarf_dealloc(dbg, filename, DW_DLA_STRING);
            }
            dwarf_srclines_dealloc_b(line_context);
        }

        dwarf_object_finish(dbg);

        return true;
    }

    std::string getParserType() final
    {
        return "Mach-O";
    }

    enum PossibleHits maxPossibleHits() final
    {
        return PossibleHits::HITS_LIMITED;
    }

    // Search for the "__go_buildinfo" section in the "__DATA" segment.
    bool isGoBinary(const std::string& filename)
    {
        size_t read_size = 0;
        auto full_file_content =
            static_cast<uint8_t*>(read_file(&read_size, "%s", filename.c_str()));
        auto hdr = reinterpret_cast<mach_header_64*>(full_file_content);
        auto cmd_ptr = full_file_content + sizeof(mach_header_64);
        for (auto i = 0; i < hdr->ncmds; i++)
        {
            auto cmd = reinterpret_cast<load_command*>(cmd_ptr);
            switch (cmd->cmd)
            {
            case LC_SEGMENT_64: {
                auto segment = reinterpret_cast<const struct segment_command_64*>(cmd_ptr);
                if (strcmp(segment->segname, "__DATA") == 0)
                {
                    auto section_ptr = cmd_ptr + sizeof(struct segment_command_64);
                    for (auto i = 0; i < segment->nsects; i++)
                    {
                        auto section = reinterpret_cast<struct section_64*>(section_ptr);
                        if (strcmp(section->sectname, "__go_buildinfo") == 0)
                        {
                            free((void*)full_file_content);
                            return true;
                        }
                        section_ptr += sizeof(*section);
                    }
                }
            }
            break;
            default:
                break;
            }
            cmd_ptr += cmd->cmdsize;
        }
        free((void*)full_file_content);
        return false;
    }

    unsigned int matchParser(const std::string& filename, uint8_t* data, size_t dataSize) final
    {
        auto hdr = reinterpret_cast<mach_header_64*>(data);

        // Don't handle big endian machines, or 32-bit binaries
        if (hdr->magic == FAT_MAGIC_64)
        {
            error("kcov doesn't support FAT binaries");
            return match_none;
        }
        if (hdr->magic == MH_MAGIC_64)
        {
            auto& conf = IConfiguration::getInstance();
            if (!conf.keyAsInt("is-go-binary") && isGoBinary(filename))
            {
                conf.setKey("is-go-binary", 1);
            }
            return match_perfect;
        }

        return match_none;
    }

    void setupParser(IFilter* filter) final
    {
        auto& conf = IConfiguration::getInstance();
        if (conf.keyAsInt("is-go-binary"))
        {
            // Do nothing, because the Go linker puts dwarf info in the binary instead of a seperated dSYM file.
            // This can be removed if this proposal passed.
            // https://github.com/golang/go/issues/62577
        }
        else
        {
            // Run dsymutil to make sure the DWARF info is avaiable
            auto dsymutil_command = fmt("dsymutil %s/%s",
                                        conf.keyAsString("binary-path").c_str(),
                                        conf.keyAsString("binary-name").c_str());
            kcov_debug(ELF_MSG, "running %s\n", dsymutil_command.c_str());

            system(dsymutil_command.c_str());
        }
    }

    // Mach-O command handlers
    void commandParseSegment64()
    {
        auto segment = reinterpret_cast<const struct segment_command_64*>(m_readPtr);

        // We only handle segments with debug info
        if (strcmp(segment->segname, "__DWARF") != 0)
        {
            return;
        }

        m_readPtr += sizeof(struct segment_command_64);
        for (auto i = 0; i < segment->nsects; i++)
        {
            parseSection();
        }
    }


    void parseSection()
    {
        auto section = reinterpret_cast<struct section_64*>(m_readPtr);

        m_dwarfSections.push_back(section);
        m_readPtr += sizeof(*section);
    }

    // libdwarf callbacks for the class

    std::vector<ILineListener*> m_lineListeners;
    std::vector<IFileListener*> m_fileListeners;

    std::string m_filename;

    // Dwarf data
    std::vector<struct section_64*> m_dwarfSections;

    size_t m_fileSize {0};
    uint8_t* m_fileData {nullptr};
    uint8_t* m_readPtr {nullptr};
};

} // namespace

MachoParser g_machoParser;
