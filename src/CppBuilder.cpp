#if defined(_WIN32)
#include "windows.h"
#endif

#if defined(__linux__)
#include <unistd.h>
#include <linux/limits.h>
#endif

#include "CppBuilder.h"
#include "SystemInterface.h"

#include <filesystem>
#include <exception>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define MOD_TIME std::filesystem::last_write_time
#define EXISTS std::filesystem::exists

#define CPPBUILD_FILE_EXT ".cbld"

// the constructer requires the arguments in the command line. From there it takes care of everything.
CppBuilder::CppBuilder(int argc, char *argv[]) : run(false), force(false), update_Needed(false)
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

    proj_Info.arch = sizeof(void *) == 8 ? X64 : X86;
    proj_Info.config = RELEASE;

    for (int i = 0; i < argc; i++)
    {
        if (strlen(argv[i]) == 7 && strcmp(argv[i], "--setup") == 0)
            setup();
        else if (strlen(argv[i]) >= 7 && memcmp(argv[i], "--path=", 7) == 0)
        {
            if (!std::filesystem::exists(argv[i] + 7))
                error("ERROR: Path \'%s\' does not exist", argv[i] + 7);
            else
                path_To_Project = std::filesystem::canonical(std::filesystem::path(argv[i] + 7)).string();
        }
        else if (strlen(argv[i]) == 10 && memcmp(argv[i], "--arch=", 7) == 0)
        {
            if (strcmp(argv[i] + 7, "x86") == 0)
                proj_Info.arch = X86;
            else if (strcmp(argv[i] + 7, "x64") == 0)
                proj_Info.arch = X64;
            else
                error("SYNTAX ERROR: Invalid architecture. Must be \'x86\' or \'x64\'");
        }
        else if (strlen(argv[i]) >= 9 && memcmp(argv[i], "--config=", 9) == 0)
        {
            if (strcmp(argv[i] + 9, "debug") == 0)
                proj_Info.config = DEBUG;
            else if (strcmp(argv[i] + 9, "release") == 0)
                proj_Info.config = RELEASE;
            else
                error("SYNTAX ERROR: Invalid architecture. Must be \'x86\' or \'x64\'");
        }
        else if (strlen(argv[i]) == 5 && strcmp(argv[i], "--run") == 0)
            run = true;
        else if (strlen(argv[i]) == 7 && strcmp(argv[i], "--force") == 0)
            force = true;
        else if (strlen(argv[i]) == 6 && strcmp(argv[i], "--help") == 0)
        {
            printf("This text is not going to help you LOL :)\n");
            exit(0);
        }
        else if (i > 0)
            error("SYNTAX ERROR: Unrecognized flag %s", argv[i]);
    }

    if (!std::filesystem::exists(std::filesystem::path(path_To_Project)))
        error("ERROR: The path \'%s\' does not exist", path_To_Project.c_str());

    if (std::filesystem::is_directory(path_To_Project))
    {
        path_To_Buildfile = "";
        auto iterator = std::filesystem::directory_iterator(path_To_Project);
        for (const auto &entry : iterator)
            if (entry.path().extension() == CPPBUILD_FILE_EXT)
            {
                path_To_Buildfile = std::filesystem::canonical(entry.path()).string();
            }
        if (path_To_Buildfile.length() == 0)
            error("ERROR: No build file found in project");
    }
    else if (std::filesystem::path(path_To_Project).extension() == CPPBUILD_FILE_EXT)
    {
        path_To_Buildfile = path_To_Project;
        path_To_Project = std::filesystem::canonical(std::filesystem::path(path_To_Buildfile).parent_path()).string();
    }
    else
    {
        error("ERROR: The specified file \'%s\' is not a build file", path_To_Project.c_str());
    }

    std::filesystem::current_path(path_To_Project);

    proj_Info.init(path_To_Buildfile);

    if (!EXISTS(proj_Info.output_Path) || MOD_TIME(path_To_Buildfile) > MOD_TIME(proj_Info.output_Path))
        update_Needed = true;

    std::filesystem::current_path(path_To_Exe);
    if (!EXISTS("cppbuild_win32_info.txt"))
        error("ERROR: Windows setup required (--setup)");

    FILE *p_File = fopen("cppbuild_win32_info.txt", "r");
    char buffer[2048];
    if (p_File)
    {
        fgets(buffer, 2048, p_File);
        buffer[strlen(buffer) - 1] = 0;
        si.system_Include_Paths = buffer;

        fgets(buffer, 2048, p_File);
        buffer[strlen(buffer) - 1] = 0;
        si.system_Lib32_Paths = buffer;

        fgets(buffer, 2048, p_File);
        buffer[strlen(buffer) - 1] = 0;
        si.system_Lib64_Paths = buffer;

        fgets(buffer, 2048, p_File);
        buffer[strlen(buffer) - 1] = 0;
        si.compiler32 = buffer;

        fgets(buffer, 2048, p_File);
        buffer[strlen(buffer) - 1] = 0;
        si.compiler64 = buffer;
    }
    std::filesystem::current_path(path_To_Project);
}

void CppBuilder::print_Project_Info(void)
{
    printf(F_WHITE F_BOLD "Project:       " F_RESET "\'%s\'\n", path_To_Project.c_str());
    printf(F_WHITE F_BOLD "Output:        " F_RESET "\'%s\' (%s)\n", proj_Info.output_Path.c_str(), proj_Info.output_Type == APP ? "Application" : (proj_Info.output_Type == LIB ? "Static Library" : "None"));
    printf(F_WHITE F_BOLD "Platform:      " F_RESET "%s\n", strcmp(OS_NAME, "win32") == 0 ? "Windows" : "Linux");
    printf(F_WHITE F_BOLD "Architecture:  " F_RESET "%s\n", proj_Info.arch == X64 ? "x64" : "x86");
    printf(F_WHITE F_BOLD "Configuration: " F_RESET "%s\n", proj_Info.config == RELEASE ? "Release" : "Debug");
    printf("\n");
}

void CppBuilder::build(void)
{
    std::string cppbuild_Path;
#if defined(_WIN32)
    cppbuild_Path = (std::filesystem::path(path_To_Exe) / "cppbuild.exe").string();
#elif defined(__linux__)
    cppbuild_Path = (std::filesystem::path(path_To_Exe) / "cppbuild").string();
#endif

    for (const auto &cmd : proj_Info.commands)
    {
        if (cmd.type == EXEC_DEP)
        {
            printf(F_CYAN F_BOLD "Building dependency \'%s\'\n", cmd.data[0].c_str());
            int ret = si.execute_Program(cppbuild_Path.c_str(), ("--path=\"" + cmd.data[0] + "\" --arch=" + (proj_Info.arch == X64 ? "x64" : "x86") + " --config=" + (proj_Info.config == RELEASE ? "release" : "debug") + (force ? " --force" : "")).c_str());
            if (ret != 0)
            {
                printf(F_YELLOW "WARNING: Build of dependecy \'%s\' failed\n\n" F_RESET, cmd.data[0].c_str());
            }
        }
        else
        {
            if (cmd.data[1].size() == 0)
            {
                printf(F_CYAN F_BOLD "Running \'%s\'\n", cmd.data[0].c_str());
                int ret = si.execute_Program(cmd.data[0].c_str(), NULL);
                printf(F_CYAN F_BOLD "Process returned with %i\n" F_RESET, ret);
            }
            else
            {
                printf(F_CYAN F_BOLD "Running \'%s\' with arguments \'%s\'\n", cmd.data[0].c_str(), cmd.data[1].c_str());
                int ret = si.execute_Program(cmd.data[0].c_str(), cmd.data[1].c_str());
                printf(F_CYAN F_BOLD "Process returned with %i\n" F_RESET, ret);
            }
        }
    }

    printf(F_BOLD "Compiling source files: \n" F_RESET);
    for (const auto &tu : proj_Info.files)
        compile(tu);
    printf("\n");

    link();
    proj_Info.save_Header_Dependencies();
    for (const auto &exportf : proj_Info.exportfs)
    {
        if (exportf.type == FILE_EXPORT)
        {
            if (EXISTS(exportf.dst_Path))
                std::filesystem::remove_all(exportf.dst_Path);
            
            if (EXISTS(exportf.src_Path)){
                std::filesystem::copy(exportf.src_Path, exportf.dst_Path);
                printf(F_CYAN "Exported \'%s\' to \'%s\'\n" F_RESET, exportf.src_Path.c_str(), exportf.dst_Path.c_str());
            }else
                printf(F_YELLOW "WARNING: Could not export file \'%s\', because it does not exist\n" F_RESET, exportf.src_Path.c_str());
        } else if (exportf.type == HEADERS_EXPORT)
        {
            if (EXISTS(exportf.dst_Path))
                std::filesystem::remove_all(exportf.dst_Path);
            
            if (!EXISTS(exportf.src_Path))
                printf(F_YELLOW "WARNING: Could not export headers, because the folder \'%s\' does not exist\n" F_RESET, exportf.src_Path.c_str());
            if (!std::filesystem::is_directory(exportf.src_Path))
                printf(F_YELLOW "WARNING: Could not export headers, because \'%s\' is not a folder\n" F_RESET, exportf.src_Path.c_str());
            auto iterator = std::filesystem::recursive_directory_iterator(exportf.src_Path);
            for(const auto& entry : iterator) {
                if(entry.is_regular_file() && (entry.path().extension() == ".h" || entry.path().extension() == ".hpp")){
                    auto dst_File = std::filesystem::path(exportf.dst_Path) / std::filesystem::proximate(entry.path(), exportf.src_Path).parent_path();
                    std::filesystem::create_directories(dst_File);
                    std::filesystem::copy(entry.path(), std::filesystem::canonical(dst_File) / entry.path().filename());
                }
            }
            printf(F_CYAN "Exported headers from \'%s\' to \'%s\'\n" F_RESET, exportf.src_Path.c_str(), exportf.dst_Path.c_str());
        }
    }
    if(proj_Info.exportfs.size() > 0)
        printf("\n");
    if (run && proj_Info.output_Type == APP)
    {
        printf(F_BOLD "Running \'%s\' ..." F_RESET "\n", proj_Info.output_Path.c_str());
        int ret = si.execute_Program(proj_Info.output_Path.c_str(), NULL);
        printf(F_BOLD "Application terminated with %i\n" F_RESET, ret);
    }
}

void CppBuilder::compile(TranslationUnit tu)
{
    printf("\'%s\'", tu.cpp_File.c_str());
    if (force || update_Needed || needs_Update(tu))
    {
        printf("\n");
        std::filesystem::create_directories(std::filesystem::path(tu.o_File).parent_path());
        if (si.compile(tu, &proj_Info) == ERROR_CODE)
        {
            error(F_BOLD "COMPILATION FAILED");
        }
        else
        {
            printf("\033[F\'%s\'" F_GREEN F_BOLD " SUCCESS\n" F_RESET, tu.cpp_File.c_str());
        }
    }
    else
    {
        printf(F_YELLOW F_BOLD " UP TO DATE\n" F_RESET);
    }
}

bool CppBuilder::needs_Update(TranslationUnit tu)
{
    if (!EXISTS(tu.o_File) || MOD_TIME(tu.cpp_File) > MOD_TIME(tu.o_File))
        return true;

    if (proj_Info.header_Dependencies.find(tu.cpp_File) == proj_Info.header_Dependencies.end())
        return true;

    const auto &header_Deps = proj_Info.header_Dependencies[tu.cpp_File];
    for (int i = 0; i < header_Deps.size(); i++)
    {
        if (!EXISTS(header_Deps[i]))
            continue;
        if (MOD_TIME(header_Deps[i]) > MOD_TIME(tu.o_File))
            return true;
    }
    return false;
}

void CppBuilder::link(void)
{
    std::filesystem::create_directories(std::filesystem::path(proj_Info.output_Path).parent_path());
    if (proj_Info.output_Type == APP)
    {
        printf(F_BOLD "Creating application \'%s\'\n" F_RESET, proj_Info.output_Path.c_str());
        if (si.link_App(&proj_Info) == ERROR_CODE)
        {
            error(F_BOLD "Creation of \'%s\' failed" F_RESET, proj_Info.output_Path.c_str());
        }
        else
        {
            printf(F_BOLD "\033[FCreating application \'%s\'" F_GREEN F_BOLD " SUCCESS\n\n" F_RESET, proj_Info.output_Path.c_str());
        }
    }
    else
    {
        printf(F_BOLD "Creating static library \'%s\'\n" F_RESET, proj_Info.output_Path.c_str());
        if (si.link_Lib(&proj_Info) == ERROR_CODE)
        {
            error(F_BOLD "Creation of \'%s\' failed" F_RESET, proj_Info.output_Path.c_str());
        }
        else
        {
            printf(F_BOLD "\033[FCreating static library \'%s\'" F_GREEN F_BOLD " SUCCESS\n\n" F_RESET, proj_Info.output_Path.c_str());
        }
    }
}

// This function
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
    fprintf(p_File, "%s/bin/Host%s/x86\n", msvc_path.string().c_str(), sizeof(void *) == 8 ? "x64" : "x86");
    fprintf(p_File, "%s/bin/Host%s/x64\n", msvc_path.string().c_str(), sizeof(void *) == 8 ? "x64" : "x86");

    fclose(p_File);

    exit(0);
#elif defined(__linux__)
    error("SYNTAX ERROR: The flag --setup can only be used on Windows");
#endif
}