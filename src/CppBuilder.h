#pragma once

#include <string>
#include "cppbuild.h"

struct CppBuilder {
    std::string path_To_Exe, path_To_Project;

    std::string system_Include_Paths = "", system_Lib32_Paths = "", system_Lib64_Paths = "";

    bool run, force;

    Config config;

    Architecture arch;

    CppBuilder(int argc, char* argv[]);

    void setup(void);
};