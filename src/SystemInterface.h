#pragma once

#include "ProjectInfo.h"

struct SystemInterface
{
    std::string system_Include_Paths = "", system_Lib32_Paths = "", system_Lib64_Paths = "", compiler32 = "", compiler64 = "";

    int compile(TranslationUnit tu, ProjectInfo *p_Proj_Info);

    int link_App(ProjectInfo *p_Proj_Info);

    int link_Lib(ProjectInfo *p_Proj_Info);

    int execute_Program(const char* prog, const char* args);
};