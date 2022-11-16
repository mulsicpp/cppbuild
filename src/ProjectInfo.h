#pragma once

#include "cppbuild.h"

#include <string>
#include <unordered_map>
#include <vector>

struct Value {
    std::string value;
    bool is_Const;
};

struct TranslationUnit {
    std::string cpp_File;
    std::string o_File;
};

struct ProjectInfo {
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

    std::vector<std::string> dependencies;

    std::string bin_Dir_Name = "";

    void init(void);
    
    void search_Source_Files(void);

    void load_Header_Dependencies(void);
    void save_Header_Dependencies(void);
    
private:
    std::string resolve_Arg(std::string line, int line_Index);

    std::string execute_Script_Line(std::string line, int line_Index);

    static void format_Line(int *argc, char **argv, char *line);

    void execute_Line(int argc, std::string args[MAX_ARG_COUNT], int line_Index);
};