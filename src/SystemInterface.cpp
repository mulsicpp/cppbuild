#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__)
#include <unistd.h>
#include <linux/limits.h>
#endif

#include "SystemInterface.h"
#include "cppbuild.h"

#include <filesystem>
#include <vector>
#include <string>
#include <string.h>

int SystemInterface::compile(TranslationUnit tu, ProjectInfo *p_Proj_Info)
{
#if defined(_WIN32)

#elif defined(__linux__)
    char buffer[1024];
    FILE *pipe = popen(("g++ -march=x86-64 " + std::string(p_Proj_Info->arch == X86 ? "-m32 " : "-m64 ") + p_Proj_Info->comp_Flags + "-c -H -o \"" + tu.o_File + "\" \"" + tu.cpp_File + "\" 2>&1").c_str(), "r");

    printf("running g++...\n");

    if (!pipe)
    {
        printf("couldn't run\n");
        return ERROR_CODE;
    }

    const unsigned int pref_length = 22;
    char *path;
    std::filesystem::path current_include;
    int include_length = 0;

    std::vector<std::string> headers;

    while (fgets(buffer, 1024, pipe))
    {
        // check include files
        if (buffer[0] == '.')
        {
            path = buffer + 1;
            while (path[0] == '.')
                path++;
            if (path[0] == ' ')
            {
                path++;
                include_length = strlen(path);
                path[include_length - 1] = 0;
                try
                {
                    current_include = std::filesystem::canonical(std::string(path));
                }
                catch (std::filesystem::filesystem_error e)
                {
                    error("path of included file \'%s\' is not valid", path);
                }
                auto rel = std::filesystem::relative(path, std::filesystem::current_path());
                if (!rel.empty() && rel.native()[0] != '.')
                {
                    headers.push_back(std::filesystem::proximate(path).string());
                    continue;
                }
                for (const auto &include_Path : p_Proj_Info->include_Paths)
                {
                    auto rel = std::filesystem::relative(path, include_Path);
                    if (!rel.empty() && rel.native()[0] != '.')
                    {
                        headers.push_back(std::filesystem::proximate(path).string());
                        break;
                    }
                }
            }
        }
        else if (strcmp(buffer, "Multiple include guards may be useful for:\n") == 0)
        {
            continue;
        }
        else
        {
            if (buffer[0] != '/')
            {
                printf("%s", buffer);
            }
        }
    }

    printf("header count: %i\n", headers.size());
    p_Proj_Info->header_Dependencies[tu.cpp_File] = headers;

    int rc = pclose(pipe);
    return rc == 0 ? UPDATED_CODE : ERROR_CODE;
#endif
}

int SystemInterface::link_App(ProjectInfo *p_Proj_Info)
{
    std::string o_Files = "";
    for (const auto &tu : p_Proj_Info->files)
        o_Files += "\"" + tu.o_File + "\" ";
#if defined(_WIN32)

#elif defined(__linux__)
    char buffer[1024];
    FILE *pipe = popen(("g++ -march=x86-64 " + std::string(p_Proj_Info->arch == X86 ? "-m32 " : "-m64 ") + "-o \"" + p_Proj_Info->output_Path + "\" " + o_Files + p_Proj_Info->link_Flags + + " " + p_Proj_Info->libs + " 2>&1").c_str(), "r");

    printf("running g++...\n");

    if (!pipe)
    {
        printf("couldn't run\n");
        return ERROR_CODE;
    }

    while (fgets(buffer, 1024, pipe))
        printf("%s", buffer);

    int rc = pclose(pipe);
    return rc == 0 ? UPDATED_CODE : ERROR_CODE;
#endif
}

int SystemInterface::link_Lib(ProjectInfo *p_Proj_Info)
{
    std::string o_Files = "";
    for (const auto &tu : p_Proj_Info->files)
        o_Files += "\"" + tu.o_File + "\" ";
#if defined(_WIN32)

#elif defined(__linux__)
    char buffer[1024];
    FILE *pipe = popen(("ar rcs \"" + p_Proj_Info->output_Path + "\" " + o_Files + " 2>&1").c_str(), "r");

    printf("running g++...\n");

    if (!pipe)
    {
        printf("couldn't run\n");
        return ERROR_CODE;
    }

    while (fgets(buffer, 1024, pipe))
        printf("%s", buffer);

    int rc = pclose(pipe);
    return rc == 0 ? UPDATED_CODE : ERROR_CODE;
#endif
}

int SystemInterface::execute_Program(const char *prog, const char *args)
{
#if defined(_WIN32)

#elif defined(__linux__)
    char buffer[512];
    FILE *pipe = popen(("\'" + std::string(prog) + "\' " + std::string(args == NULL ? "" : args)).c_str(), "r");

    if (!pipe)
    {
        printf(RED "error : process wasn't able to run\n" C_RESET);
        exit(-1);
    }

    while (!feof(pipe))
    {
        if (fgets(buffer, 512, pipe) != nullptr)
            printf("%s", buffer);
    }

    int rc = pclose(pipe);
    return rc;
#endif
}