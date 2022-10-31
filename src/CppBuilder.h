#pragma once

#include <string>
#include "cppbuild.h"
#include "ProjectInfo.h"

struct CppBuilder {
    std::string path_To_Exe, path_To_Project;

    std::string system_Include_Paths = "", system_Lib32_Paths = "", system_Lib64_Paths = "";

    bool run, force;

    ProjectInfo proj_Info;

    CppBuilder(int argc, char* argv[]);

    void build(void);

    void setup(void);

private:
    void compile(TranslationUnit tu);

    bool needs_Update(TranslationUnit tu);

    void link(void);
};