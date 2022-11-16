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
    variables["_platform"] = {OS_NAME, true};
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
            else if (line.at(0) == '!')
            {
                line = line.substr(1);
                trim(line);
                if ((line = execute_Script_Line(line, i + 1)).size() == 0)
                    continue;
            }

            strcpy(line_cstr, line.c_str());
            format_Line(&argc, argv, line_cstr);
            for (int i = 0; i < argc; i++)
                args[i] = resolve_Arg(argv[i], i + 1);

            execute_Line(argc, args, i + 1);
        }
    }
}

std::string ProjectInfo::resolve_Arg(std::string arg, int line_Index)
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
            error("(LINE: %i) ERROR: Varibale '%s' doesn't exist", line_Index, matches[1].str().c_str());
        }
    }

    arg = std::regex_replace(arg, APP_FUNC_REG, OS_APP_FORMAT);
    arg = std::regex_replace(arg, LIB_FUNC_REG, OS_LIB_FORMAT);
    arg = std::regex_replace(arg, DLL_FUNC_REG, OS_DLL_FORMAT);

    return arg;
}

std::string ProjectInfo::execute_Script_Line(std::string line, int line_Index)
{
    char buffer[MAX_LINE_LENGTH];
    strcpy(buffer, line.c_str());
    int argc;
    char *argv[MAX_ARG_COUNT];
    format_Line(&argc, argv, buffer);
    if (argc == 4)
    {
        if (strcmp(argv[0], "let") == 0)
        {
            for (int i = 0; argv[1][i] != 0; i++)
            {
                char c = argv[1][i];
                if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && (c < '0' || c > '9') && c != '_')
                    error("(LINE: %i) SYNTAX ERROR: Variable name \'%s\' is invalid. Must contain alpha-numeric characters", line_Index, argv[1]);
            }
            if (strcmp(argv[2], "=") == 0)
            {
                if (variables.find(argv[1]) != variables.end())
                    if (!variables[argv[1]].is_Const)
                        variables[argv[1]].value = argv[3];
                    else
                        error("(LINE: %i) ERROR: Cannot overwrite constant variable \'%s\'", line_Index, argv[1]);
                else
                    variables[argv[1]] = {argv[3], false};
            }
            else if (strcmp(argv[2], "?=") == 0)
            {
                if (variables.find(argv[1]) == variables.end())
                    variables[argv[1]] = {argv[3], false};
            }
            else
            {
                error("(LINE: %i) SYNTAX ERROR: Unexpected token \'%s\'. Should be \'=\' or \'?=\'", line_Index, argv[2]);
            }
        }
        else if (strcmp(argv[0], "const") == 0)
        {
            for (int i = 0; argv[1][i] != 0; i++)
            {
                char c = argv[1][i];
                if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && (c < '0' || c > '9') && c != '_')
                    error("(LINE: %i) SYNTAX ERROR: Variable name \'%s\' is invalid. Must contain alpha-numeric characters", line_Index, argv[1]);
            }
            if (strcmp(argv[2], "=") == 0)
            {
                if (variables.find(argv[1]) != variables.end())
                    if(variables[argv[1]].is_Const)
                        error("(LINE: %i) ERROR: Cannot overwrite constant variable \'%s\'", line_Index, argv[1]);
                    else
                        error("(LINE: %i) ERROR: Cannot redefine variable \'%s\' as const", line_Index, argv[1]);
                else
                    variables[argv[1]] = {argv[3], true};
            }
            else
            {
                error("(LINE: %i) SYNTAX ERROR: Unexpected token \'%s\'. Should be \'=\' or \'?=\'", line_Index, argv[2]);
            }
        }
    }
    return "";
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
            error("(LINE: %i) SYNTAX ERROR: \'%s\' is not allowed as first argument for \'output\'. Has to be \'app\' or \'lib\'", line_Index, args[1].c_str());
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
        include_Paths.push_back(args[1]);
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
        {
            args[2] = std::regex_replace(args[2], std::regex("\""), "\\\"");
            comp_Flags += OS_MACRO(args[1], args[2]);
        }
    }
    else if (args[0] == "std")
    {
        comp_Flags += OS_STD(args[1]);
    }
    else if (args[0] == "require")
    {
        dependencies.push_back(args[1]);
    }
    else
    {
        error("(LINE: %i) SYNTAX ERROR: Unrecognized command \'%s\'", line_Index, args[0].c_str());
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
            while (std::getline(ss, header, ';'))
            {
                if (header.length() > 0)
                {
                    headers.push_back(header);
                }
            }
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