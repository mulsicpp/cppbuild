#pragma once

#include <string>
#include "cppbuild.h"
#include "ProjectInfo.h"
#include "SystemInterface.h"

struct CppBuilder {
    std::string path_To_Exe, path_To_Project;

    bool run, force;

    ProjectInfo proj_Info;
    SystemInterface si;

    CppBuilder(int argc, char* argv[]);

    void build(void);

    void setup(void);

private:
    void compile(TranslationUnit tu);

    bool needs_Update(TranslationUnit tu);

    void link(void);
};