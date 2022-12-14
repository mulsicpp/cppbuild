#pragma once

#include "cppbuild.h"

#include <string>
#include <unordered_map>
#include <vector>

struct Value
{
    std::string value;
    bool is_Const;
};

struct TranslationUnit
{
    std::string cpp_File;
    std::string o_File;
};

#define EXEC_DEP 0
#define EXEC_CMD 1

struct Command
{
    uint8_t type;
    std::string data[2];
};

enum ExportType {
    FILE_EXPORT,
    HEADERS_EXPORT
};

struct Export {
    ExportType type;
    std::string src_Path;
    std::string dst_Path;
};

void format_Line(int *argc, char **argv, char *line);

std::string &trim(std::string &str);

struct ProjectInfo
{
    std::string output_Path;
    OutputType output_Type = NONE;

    Config config;
    Architecture arch;

    std::string comp_Flags = "";
    std::string link_Flags = "";
    std::string libs = "";
    std::vector<std::string> include_Paths;

    std::unordered_map<std::string, Value> variables;

    std::vector<TranslationUnit> files;
    std::unordered_map<std::string, std::vector<std::string>> header_Dependencies;

    std::vector<Command> commands;
    std::vector<std::string> exportcs;
    std::vector<Export> exportfs;

    std::string bin_Dir_Name = "";

    void init(const std::string& build_File);

    void search_Source_Files(void);

    void load_Header_Dependencies(void);
    void save_Header_Dependencies(void);

    void execute_Exported_Line(int argc, std::string args[MAX_ARG_COUNT], int line_Index);

private:
    std::string resolve_Line(std::string line, int line_Index);
    std::string resolve_Arg(std::string line, int line_Index);


    void execute_Line(int argc, std::string args[MAX_ARG_COUNT], int line_Index);
};