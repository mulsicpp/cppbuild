#if defined(_WIN32)
#include "Windows.h"
#endif

#include "ProjectInfo.h"
#include "SystemInterface.h"

#include <fstream>
#include <regex>
#include <algorithm>
#include <filesystem>
#include <sstream>

#define CONDITION_FLAG 0x1
#define IF_FLAG 0x2

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

    std::vector<uint8_t> if_Stack;

    for (int i = 0; std::getline(in, line); i++)
    {
        if (!std::regex_match(line, _EMPTY_LINE_REG))
        {
            trim(line);
            if (line.at(0) == '#')
                continue;

            line = resolve_Line(line, i + 1);

            trim(line);
            if (line.length() == 0)
                continue;

            strcpy(line_cstr, line.c_str());
            format_Line(&argc, argv, line_cstr);
            if (if_Stack.size() > 0)
            {
                if (((if_Stack.back() & IF_FLAG) != 0) && strcmp(argv[0], "else") == 0)
                {
                    if (argc != 1)
                        error("(LINE: %s) SYNTAX ERROR: \'else\' doesn't take any arguments");
                    if_Stack.back() &= ~IF_FLAG;
                    continue;
                }
                else if (strcmp(argv[0], "endif") == 0)
                {
                    if (argc != 1)
                        error("(LINE: %s) SYNTAX ERROR: \'endif\' doesn't take any arguments");
                    if_Stack.pop_back();
                    continue;
                }
                else if (if_Stack.back() & CONDITION_FLAG == 0)
                    continue;
            }
            for (int j = 0; j < argc; j++)
                args[j] = resolve_Arg(argv[j], i + 1);

            if (args[0] == "if")
            {
                if (argc != 4)
                    error("(LINE: %s) SYNTAX ERROR: Invalid if-statement. Has to look like: if <arg1> <operator> <arg2>");

                if (args[2] == "=")
                    if_Stack.push_back(IF_FLAG | (args[1] == args[3] ? CONDITION_FLAG : 0));
                else if (args[2] == "!=")
                    if_Stack.push_back(IF_FLAG | (args[1] != args[3] ? CONDITION_FLAG : 0));
                else
                    error("(LINE: %s) SYNTAX ERROR: Invalid operator \'%s\'. Only \'=\' and \'!=\' are allowed");
                continue;
            }
            if (if_Stack.size() == 0 || if_Stack.back() == 0 || if_Stack.back() == (IF_FLAG | CONDITION_FLAG))
                execute_Line(argc, args, i + 1);
        }
    }
}

#define REG_ARG(x, y) "([^\\" x "\\" y "]*?)"

std::string ProjectInfo::resolve_Line(std::string line, int line_Index)
{
    std::smatch matches;

    while (std::regex_search(line.cbegin(), line.cend(), matches, std::regex("\\$\\[(\\w*)\\]")))
    {
        if (variables.find(matches[1].str()) != variables.end())
        {
            Value var = variables[matches[1].str()];
            line = std::regex_replace(line, std::regex("\\$\\[" + matches[1].str() + "\\]"), var.value);
        }
        else
        {
            error("(LINE: %i) ERROR: Varibale '%s' doesn't exist", line_Index, matches[1].str().c_str());
        }
    }

    while (std::regex_search(line.cbegin(), line.cend(), matches, std::regex("\\[ *" REG_ARG("[", "]") "  *(=|!=)  *" REG_ARG("[", "]") "  *\\?  *" REG_ARG("[", "]") " *(?: :  *" REG_ARG("[", "]") ")? *\\]")))
    {
        if (matches.size() == 6)
        {
            std::string arg1 = matches[1].str(), op = matches[2].str(), arg2 = matches[3].str(), if_Block = matches[4].str(), else_Block = matches[5].str();
            if (op == "=" ? arg1 == arg2 : arg1 != arg2)
            {
                line = std::regex_replace(line, std::regex("\\[ *" + arg1 + "  *" + op + "  *" + arg2 + "  *\\?  *" + if_Block + " *" + (matches[5].matched ? " :  *" + else_Block : "") + "\\]"), if_Block);
            }
            else
            {
                line = std::regex_replace(line, std::regex("\\[ *" + arg1 + "  *" + op + "  *" + arg2 + "  *\\?  *" + if_Block + " *" + (matches[5].matched ? " :  *" + else_Block : "") + "\\]"), else_Block);
            }
        }
    }

    return line;
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

    if (args[0] == "let")
    {
        if (argc != 4)
            error("(LINE: %i) SYNTAX ERROR: Invalid number of arguments for command \'let\'", line_Index);
        auto data = args[1].data();
        for (int i = 0; data[i] != 0; i++)
        {
            char c = data[i];
            if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && (c < '0' || c > '9') && c != '_')
                error("(LINE: %i) SYNTAX ERROR: Variable name \'%s\' is invalid. Must contain alpha-numeric characters", line_Index, args[1]);
        }
        if (args[2] == "=")
        {
            if (variables.find(args[1]) != variables.end())
                if (!variables[args[1]].is_Const)
                    variables[args[1]].value = args[3];
                else
                    error("(LINE: %i) ERROR: Cannot overwrite constant variable \'%s\'", line_Index, args[1]);
            else
                variables[args[1]] = {args[3], false};
        }
        else if (args[2] == "?=")
        {
            if (variables.find(args[1]) == variables.end())
                variables[args[1]] = {args[3], false};
        }
        else
        {
            error("(LINE: %i) SYNTAX ERROR: Unexpected token \'%s\'. Should be \'=\' or \'?=\'", line_Index, args[2]);
        }
    }
    else if (args[0] == "const")
    {
        if (argc != 4)
            error("(LINE: %i) SYNTAX ERROR: Invalid number of arguments for command \'let\'", line_Index);
        auto data = args[1].data();
        for (int i = 0; data[i] != 0; i++)
        {
            char c = data[i];
            if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && (c < '0' || c > '9') && c != '_')
                error("(LINE: %i) SYNTAX ERROR: Variable name \'%s\' is invalid. Must contain alpha-numeric characters", line_Index, args[1]);
        }
        if (args[2] == "=")
        {
            if (variables.find(args[1]) != variables.end())
                if (variables[args[1]].is_Const)
                    error("(LINE: %i) ERROR: Cannot overwrite constant variable \'%s\'", line_Index, args[1]);
                else
                    error("(LINE: %i) ERROR: Cannot redefine variable \'%s\' as const", line_Index, args[1]);
            else
                variables[args[1]] = {args[3], true};
        }
        else
        {
            error("(LINE: %i) SYNTAX ERROR: Unexpected token \'%s\'. Should be \'=\' or \'?=\'", line_Index, args[2]);
        }
    }
    else if (args[0] == "output")
    {
        if (argc != 3)
            error("(LINE: %i) SYNTAX ERROR: Invalid number of arguments for command \'output\'", line_Index);
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
        if (argc < 2)
            error("(LINE: %i) SYNTAX ERROR: Invalid number of arguments for command \'ignore\'", line_Index);
        for (int j = 1; j < argc; j++)
        {
#if defined(_WIN32)
            std::replace(args[j].begin(), args[j].end(), '/', '\\');
#endif
            for (int i = 0; i < files.size();)
                if (files[i].cpp_File == args[j])
                {
                    files.erase(files.begin() + i);
                }
                else
                    i++;
        }
    }
    else if (args[0] == "incpath")
    {
        if (argc < 2)
            error("(LINE: %i) SYNTAX ERROR: Invalid number of arguments for command \'incpath\'", line_Index);
        for (int i = 1; i < argc; i++)
        {
            comp_Flags += OS_INCLUDE_PATH(args[i]);
            include_Paths.push_back(args[i]);
        }
    }
    else if (args[0] == "lib")
    {
        if (argc < 2)
            error("(LINE: %i) SYNTAX ERROR: Invalid number of arguments for command \'lib\'", line_Index);
        for (int i = 1; i < argc; i++)
            libs += args[i] + " ";
    }
    else if (args[0] == "linklib")
    {
        if (argc < 2)
            error("(LINE: %i) SYNTAX ERROR: Invalid number of arguments for command \'linklib\'", line_Index);
        for (int i = 1; i < argc; i++)
            link_Flags += OS_LINK_LIBRARY(args[i]);
    }
    else if (args[0] == "libpath")
    {
        if (argc < 2)
            error("(LINE: %i) SYNTAX ERROR: Invalid number of arguments for command \'libpath\'", line_Index);
        for (int i = 1; i < argc; i++)
            link_Flags += OS_LIBRARY_PATH(args[i]);
    }
    else if (args[0] == "define")
    {
        if (argc == 2)
            comp_Flags += OS_DEFINE(args[1]);
        else if (argc == 3)
        {
            args[2] = std::regex_replace(args[2], std::regex("\""), "\\\"");
            comp_Flags += OS_MACRO(args[1], args[2]);
        }
        else
        {
            error("(LINE: %i) SYNTAX ERROR: Invalid number of arguments for command \'define\'", line_Index);
        }
    }
    else if (args[0] == "std")
    {
        if (argc != 2)
            error("(LINE: %i) SYNTAX ERROR: Invalid number of arguments for command \'std\'", line_Index);
        comp_Flags += OS_STD(args[1]);
    }
    else if (args[0] == "require")
    {
        if (argc < 2)
            error("(LINE: %i) SYNTAX ERROR: Invalid number of arguments for command \'require\'", line_Index);
        for (int i = 1; i < argc; i++)
            commands.push_back({EXEC_DEP, {args[i]}});
    }
    else if (args[0] == "execute")
    {
        if (argc == 2)
            commands.push_back({EXEC_CMD, {args[1], NULL}});
        else if (argc == 3)
            commands.push_back({EXEC_CMD, {args[1], args[2]}});
        else
            error("(LINE: %i) SYNTAX ERROR: Invalid number of arguments for command \'execute\'", line_Index);
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