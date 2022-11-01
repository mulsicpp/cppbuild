#if defined(_WIN32)
#include "Windows.h"
#endif

#include "ProjectInfo.h"

#include <fstream>
#include <regex>
#include <algorithm>
#include <filesystem>
#include <sstream>

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
    variables["_platform"] = { OS_NAME, true};
    variables["_arch"] = {arch == X64 ? "x64" : "x86", true};
    variables["_config"] = {config == RELEASE ? "release" : "debug", true};

    bin_Dir_Name = std::string(OS_NAME) + (arch == X64 ? "_x64" : "_x86") + (config == RELEASE ? "_release" : "_debug");

    if (!std::filesystem::exists(".cppbuild"))
        std::filesystem::create_directory(".cppbuild");
#if defined(_WIN32)
    int attributes = GetFileAttributesA(".cppbuild");
    SetFileAttributesA(".cppbuild", attributes | FILE_ATTRIBUTE_HIDDEN);
#endif

    search_Source_Files();
    load_Header_Dependencies();

    // open build file
    std::ifstream in("cppbuild");

    std::string line;
    char line_cstr[MAX_LINE_LENGTH];

    int argc;
    char *argv[MAX_ARG_COUNT];
    std::string args[MAX_ARG_COUNT];

    for (int i = 0; std::getline(in, line); i++)
    {
        if (!std::regex_match(line, _EMPTY_LINE_REG))
        {
            trim(line);
            if (line.at(0) == '#')
                continue;

            strcpy(line_cstr, line.c_str());
            format_Line(&argc, argv, line_cstr);

            printf("%s\n", line.c_str());
            for (int i = 0; i < argc; i++)
                args[i] = resolve_Arg(argv[i]);

            execute_Line(argc, args, i + 1);
        }
    }

    for (const auto &file : files)
        printf("cpp: %s >>> o: %s\n", file.cpp_File.c_str(), file.o_File.c_str());

}

std::string ProjectInfo::resolve_Arg(std::string arg)
{
    std::smatch matches;

    while (std::regex_search(arg.cbegin(), arg.cend(), matches, std::regex("\\$\\{(\\w*)\\}")))
    {
        if (variables.find(matches[1].str()) != variables.end())
        {
            Value var = variables[matches[1].str()];
            arg = std::regex_replace(arg, std::regex("\\$\\{" + matches[1].str() + "\\}"), var.value);
        }
        else
        {
            error("varibale '%s' doesn't exist", matches[1].str().c_str());
        }
    }

    arg = std::regex_replace(arg, APP_FUNC_REG, OS_APP_FORMAT);
    arg = std::regex_replace(arg, LIB_FUNC_REG, OS_LIB_FORMAT);
    arg = std::regex_replace(arg, DLL_FUNC_REG, OS_DLL_FORMAT);

    return arg;
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

void ProjectInfo::execute_Line(int argc, std::string args[MAX_ARG_COUNT], int line_Index)
{
    if (args[0] == "output")
    {
        if (args[1] == "app")
        {
            output_Type = APP;
        }
        else if (args[1] == "lib")
        {
            output_Type = LIB;
        }
        else
        {
            error("SYNTAX ERROR in line %i: \'%s\' is not allowed as first argument for \'output\'. Has to be \'app\' or \'lib\'", line_Index, args[1].c_str());
        }
#if defined(_WIN32)
        std::replace(args[2].begin(), args[2].end(), '/', '\\');
#endif
        output_Path = args[2];
    }
    else if (args[0] == "ignore")
    {
#if defined(_WIN32)
        std::replace(args[1].begin(), args[1].end(), '/', '\\');
#endif
        for (int i = 0; i < files.size();)
            if (files[i].cpp_File == args[1])
            {
                files.erase(files.begin() + i);
            }
            else
                i++;
    }
    else if (args[0] == "incpath")
    {
        comp_Flags += OS_INCLUDE_PATH(args[1]);
    }
    else if (args[0] == "lib")
    {
        libs += args[1] + " ";
    }
    else if (args[0] == "linklib")
    {
        link_Flags += OS_LINK_LIBRARY(args[1]);
    }
    else if (args[0] == "libpath")
    {
        link_Flags += OS_LIBRARY_PATH(args[1]);
    }
    else if (args[0] == "define")
    {
        if (argc == 2)
            comp_Flags += OS_DEFINE(args[1]);
        else
            comp_Flags += OS_MACRO(args[1], args[2]);
    }
    else if (args[0] == "std")
    {
        comp_Flags += OS_STD(args[1]);
    }
    else if (args[0] == "require")
    {
        dependencies.push_back(args[1]);
    }
}

void ProjectInfo::search_Source_Files(void)
{
    for (const auto &entry : std::filesystem::recursive_directory_iterator("."))
    {
        if (entry.is_regular_file() && (entry.path().extension() == ".cpp" || entry.path().extension() == ".c"))
        {
            TranslationUnit u;
            u.cpp_File = std::filesystem::proximate(entry.path()).string();
            u.o_File = (std::filesystem::path(".cppbuild" OS_DIR_SEPARATOR + bin_Dir_Name) / std::filesystem::path(u.cpp_File).replace_extension(".o")).string();
            files.push_back(u);
        }
    }
}

void ProjectInfo::load_Header_Dependencies(void)
{
    if (std::filesystem::exists(".cppbuild/" + bin_Dir_Name + "/header_dep.txt"))
    {
        std::ifstream in(".cppbuild/" + bin_Dir_Name + "/header_dep.txt");

        std::string line, src, header;
        std::vector<std::string> headers;

        while (std::getline(in, line))
        {
            std::stringstream ss(line);
            headers.clear();
            std::getline(ss, src, ':');
            printf("src: %s\nheaders:", src.c_str());
            while (std::getline(ss, header, ';'))
            {
                if (header.length() > 0)
                {
                    headers.push_back(header);
                    printf(" %s", header.c_str());
                }
            }
            printf("\n");
            header_Dependencies[src] = headers;
        }
        in.close();
    }
}

void ProjectInfo::save_Header_Dependencies(void)
{
    std::ofstream out(".cppbuild/" + bin_Dir_Name + "/header_dep.txt");

    for (const auto &[key, value] : header_Dependencies)
    {
        out << key << ":";
        for (const std::string &header : value)
            out << header << ";";
        out << std::endl;
    }
    out.close();
}