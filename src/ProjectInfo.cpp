#include "ProjectInfo.h"

#include <fstream>
#include <regex>

const static std::regex APP_FUNC_REG("%app\\(([^\\(\\)\\[\\]]*)\\)");
const static std::regex LIB_FUNC_REG("%lib\\(([^\\(\\)\\[\\]]*)\\)");
const static std::regex DLL_FUNC_REG("%dll\\(([^\\(\\)\\[\\]]*)\\)");

void ProjectInfo::init(void)
{
    // set build constants
#if defined(_WIN32)
    variables["_platform"] = {"win32", true};
#elif defined(__linux__)
    variables["_platform"] = {"linux", true};
#endif
    variables["_arch"] = {arch == X64 ? "x64" : "x86", true};
    variables["_config"] = {config == RELEASE ? "release" : "debug", true};

    // open build file
    std::ifstream in("cppbuild");

    std::string line;

    while (std::getline(in, line))
    {
        line = resolve_Line(line);
    }
}

std::string ProjectInfo::resolve_Line(std::string line)
{
    std::smatch matches;

    while (std::regex_search(line.cbegin(), line.cend(), matches, std::regex("\\${(\\w*)}")))
    {
        if (variables.find(matches[1].str()) != variables.end())
        {
            Value var = variables[matches[1].str()];
            return std::regex_replace(line, std::regex("\\$" + matches[1].str()), var.value);
        }
        else
        {
            error("varibale '%s' doesn't exist", matches[1].str().c_str());
        }
    }

    line = std::regex_replace(line, APP_FUNC_REG, OS_APP_FORMAT);
    line = std::regex_replace(line, LIB_FUNC_REG, OS_LIB_FORMAT);
    line = std::regex_replace(line, DLL_FUNC_REG, OS_DLL_FORMAT);
#if defined(_WIN32)
    line = std::regex_replace(line, std::regex("/"), "\\");
#endif
}