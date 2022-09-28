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
    OutputType output_Type;

    Config config;
    Architecture arch;

    std::string cpp_Standard;
    std::string additional_Include_Paths;
    std::string additional_Lib_Paths;
    std::string libs;

    std::string defines;

    std::unordered_map<std::string, Value> variables;

    std::vector<TranslationUnit> files;
    std::unordered_map<std::string, std::vector<std::string>> header_Dependencies;

    std::vector<std::string> dependencies;

    void init(void);
private:
    std::string resolve_Line(std::string line);
};