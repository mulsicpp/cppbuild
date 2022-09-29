#include "ProjectInfo.h"

#include <fstream>
#include <regex>

const static std::regex APP_FUNC_REG("\\$app\\(([^\\(\\)\\[\\]]*)\\)");
const static std::regex LIB_FUNC_REG("\\$lib\\(([^\\(\\)\\[\\]]*)\\)");
const static std::regex DLL_FUNC_REG("\\$dll\\(([^\\(\\)\\[\\]]*)\\)");

const static std::regex _EMPTY_LINE_REG("^[ \\r\\n\\t]*$");

static std::string &trim(std::string &str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch)
                                        { return !std::isspace(ch); }));

    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch)
                           { return !std::isspace(ch); })
                  .base(),
              str.end());

    return str;
}

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
    char line_cstr[MAX_LINE_LENGTH];

    int argc;
    char *argv[MAX_ARG_COUNT];

    while (std::getline(in, line))
    {
        if (!std::regex_match(line, _EMPTY_LINE_REG))
        {
            trim(line);
            if (line.at(0) == '#')
                continue;
            try
            {
                line = resolve_Line(line);
            }
            catch (std::exception e)
            {
                printf("exc: %s\n", e.what());
            }
            printf("%s\n", line.c_str());
            strcpy(line_cstr, line.c_str());

            format_Line(&argc, argv, line_cstr);
            execute_Line(argc, argv);
        }
    }
}

std::string ProjectInfo::resolve_Line(std::string line)
{
    std::smatch matches;

    while (std::regex_search(line.cbegin(), line.cend(), matches, std::regex("\\$\\{(\\w*)\\}")))
    {
        if (variables.find(matches[1].str()) != variables.end())
        {
            Value var = variables[matches[1].str()];
            line = std::regex_replace(line, std::regex("\\$\\{" + matches[1].str() + "\\}"), var.value);
        }
        else
        {
            error("varibale '%s' doesn't exist", matches[1].str().c_str());
        }
    }

    line = std::regex_replace(line, APP_FUNC_REG, OS_APP_FORMAT);
    line = std::regex_replace(line, LIB_FUNC_REG, OS_LIB_FORMAT);
    line = std::regex_replace(line, DLL_FUNC_REG, OS_DLL_FORMAT);

    return line;
}

void ProjectInfo::format_Line(int *argc, char **argv, char *line)
{

    size_t str_Length = strlen(line);

    bool ignore = false;

    for (int i = 0; i < str_Length; i++)
    {
        if (!ignore && (line[i] == ' ' || line[i] == '\n' || line[i] == '\t' || line[i] == '\r'))
            line[i] = 0;
        else if (line[i] == '\"')
        {
            ignore = !ignore;
            line[i] = 0;
        }
        else if (line[i] == '\\')
        {
            memcpy(line + i, line + i + 1, str_Length - i);
        }
    }
    *argc = 0;

    if (str_Length > 0 && line[0] != 0)
    {
        argv[0] = line;
        (*argc)++;
    }

    for (int i = 1; i < str_Length && *argc < MAX_ARG_COUNT; i++)
    {
        if (line[i] != 0 && line[i - 1] == 0)
        {
            argv[*argc] = line + i;
            (*argc)++;
        }
    }
}

#define TEST_ARG(arg, val) if (strcmp(arg, val) == 0)

void ProjectInfo::execute_Line(int argc, char **argv)
{
    TEST_ARG(argv[0], "output")
    {
        TEST_ARG(argv[1], "app")
        {
            output_Type = APP;
        }
        else TEST_ARG(argv[1], "lib")
        {
            output_Type = LIB;
        }
#if defined(_WIN32)
        for (int i = 0; argv[2][i] != 0; i++)
            if (argv[2][i] == '/')
                argv[2][i] = '\\';
#endif
        output_Path = std::string(argv[2]);
    }
}