#if defined(_WIN32)
#include "windows.h"
#endif

#if defined(__linux__)
#include <unistd.h>
#include <linux/limits.h>
#endif

#include "CppBuilder.h"
#include "native_commands.h"

#include <filesystem>
#include <exception>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

CppBuilder::CppBuilder(int argc, char *argv[]) : run(false), force(false)
{
#if defined(_WIN32)
    char result[MAX_PATH];
    GetModuleFileNameA(NULL, result, MAX_PATH);
#endif

#if defined(__linux__)
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
#endif
    path_To_Exe = std::filesystem::canonical(std::filesystem::path(result).parent_path()).string();

    path_To_Project = std::filesystem::canonical(std::filesystem::current_path()).string();

    arch = sizeof(void *) == 8 ? X64 : X86;

    for (int i = 1; i < argc; i++)
    {
        if (strlen(argv[i]) == 7 && strcmp(argv[i], "--setup") == 0)
            setup();
        else if (strlen(argv[i]) >= 7 && memcmp(argv[i], "--path=", 7) == 0)
        {
            if (!std::filesystem::exists(argv[i] + 7))
                error("ERROR: Project path does not exist");
            else if (!std::filesystem::is_directory(argv[i] + 7))
                error("ERROR: Project path is not a directory");
            else
                path_To_Project = std::filesystem::canonical(std::filesystem::path(argv[i] + 7)).string();
        }
        else if (strlen(argv[i]) == 10 && memcmp(argv[i], "--arch=", 7) == 0)
        {
            if (strcmp(argv[i] + 7, "x86") == 0)
                arch = X86;
            else if (strcmp(argv[i] + 7, "x64") == 0)
                arch = X64;
            else
                error("SYNTAX ERROR: Invalid architecture. Must be \'x86\' or \'x64\'");
        }
        else if (strlen(argv[i]) == 5 && strcmp(argv[i], "--run") == 0)
            run = true;
        else if (strlen(argv[i]) == 7 && strcmp(argv[i], "--force") == 0)
            force = true;
        else
            error("SYNTAX ERROR: Unrecognized flag %s", argv[i]);
    }

    if (!std::filesystem::exists(std::filesystem::path(path_To_Project) += "/cppbuild") || !std::filesystem::is_regular_file(std::filesystem::path(path_To_Project) += "/cppbuild"))
        error("ERROR: \'cppbuild\' file not present in project");
}

void CppBuilder::setup()
{
#if defined(_WIN32)

    std::filesystem::current_path(path_To_Exe);
    FILE *p_File;
    fopen_s(&p_File, "cppbuild_win32_info.txt", "w");
    char buffer[MAX_PATH];
    printf("MSVC path: ");
    fgets(buffer, MAX_PATH, stdin);
    buffer[strlen(buffer) - 1] = 0;
    auto msvc_path = std::filesystem::path(buffer);

    printf("Windows Kits path: ");
    fgets(buffer, MAX_PATH, stdin);
    buffer[strlen(buffer) - 1] = 0;
    auto win_kits_path = std::filesystem::path(buffer);

    fputs((std::string("/I\"") + (msvc_path / "include").string() + "\" ").c_str(), p_File);
    fputs((std::string("/I\"") + (msvc_path / "atlmfc/include").string() + "\" ").c_str(), p_File);

    for (const auto &subfolder : std::filesystem::directory_iterator(win_kits_path / "Include"))
        for (const auto &entry : std::filesystem::directory_iterator(subfolder.path()))
            fputs((std::string("/I\"") + entry.path().string() + "\" ").c_str(), p_File);
    fprintf(p_File, "\n");

    fputs((std::string("/libpath:\"") + (msvc_path / "lib/x86").string() + "\" ").c_str(), p_File);
    fputs((std::string("/libpath:\"") + (msvc_path / "atlmfc/lib/x86").string() + "\" ").c_str(), p_File);

    for (const auto &subfolder : std::filesystem::directory_iterator(win_kits_path / "Lib"))
    {
        fputs((std::string("/libpath:\"") + subfolder.path().string() + "/um/x86\" ").c_str(), p_File);
        fputs((std::string("/libpath:\"") + subfolder.path().string() + "/ucrt/x86\" ").c_str(), p_File);
    }
    fprintf(p_File, "\n");

    fputs((std::string("/libpath:\"") + (msvc_path / "lib/x64").string() + "\" ").c_str(), p_File);
    fputs((std::string("/libpath:\"") + (msvc_path / "atlmfc/lib/x64").string() + "\" ").c_str(), p_File);

    for (const auto &subfolder : std::filesystem::directory_iterator(win_kits_path / "Lib"))
    {
        fputs((std::string("/libpath:\"") + subfolder.path().string() + "/um/x64\" ").c_str(), p_File);
        fputs((std::string("/libpath:\"") + subfolder.path().string() + "/ucrt/x64\" ").c_str(), p_File);
    }
    fprintf(p_File, "\n");
    fprintf(p_File, "%s/bin/Host%s/x86\n", msvc_path.string().c_str(), sizeof(void*) == 8 ? "x64" : "x86");
    fprintf(p_File, "%s/bin/Host%s/x64\n", msvc_path.string().c_str(), sizeof(void*) == 8 ? "x64" : "x86");

    fclose(p_File);

    exit(0);
#elif defined(__linux__)
    error("SYNTAX ERROR: The flag --setup can only be used on Windows");
#endif
}