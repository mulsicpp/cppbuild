#if defined(_WIN32)
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
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

#if defined(_WIN32)

static FILE *win_popen(const char *proc, char *args, HANDLE *out, PROCESS_INFORMATION *pi)
{
    HANDLE outWrite = NULL;

    SECURITY_ATTRIBUTES saAttr;

    // Set the bInheritHandle flag so pipe handles are inherited.

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDOUT.

    if (!CreatePipe(out, &outWrite, &saAttr, 0))
        exit(-1);

    // Ensure the read handle to the pipe for STDOUT is not inherited.

    if (!SetHandleInformation(*out, HANDLE_FLAG_INHERIT, 0))
        exit(-1);

    STARTUPINFOA si;
    BOOL bSuccess = FALSE;

    // Set up members of the PROCESS_INFORMATION structure.

    ZeroMemory(pi, sizeof(PROCESS_INFORMATION));

    // Set up members of the STARTUPINFO structure.
    // This structure specifies the STDIN and STDOUT handles for redirection.

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = outWrite;
    si.hStdOutput = outWrite;
    si.hStdInput = NULL;
    si.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process.

    bSuccess = CreateProcessA(proc,
                              args, // command line
                              NULL, // process security attributes
                              NULL, // primary thread security attributes
                              TRUE, // handles are inherited
                              0,    // creation flags
                              NULL, // use parent's environment
                              NULL, // use parent's current directory
                              &si,  // STARTUPINFO pointer
                              pi);  // receives PROCESS_INFORMATION

    if (bSuccess)
    {
        CloseHandle(outWrite);

        int nHandle = _open_osfhandle((long)*out, _O_RDONLY);

        if (nHandle != -1)
        {
            FILE *p_file = _fdopen(nHandle, "r");
            return p_file;
        }
    }
    return NULL;
}

static int win_pclose(HANDLE *out, PROCESS_INFORMATION *pi)
{
    // Wait for the process to exit
    WaitForSingleObject(pi->hProcess, INFINITE);

    // Process has exited - check its exit code
    DWORD exitCode;
    GetExitCodeProcess(pi->hProcess, &exitCode);

    // At this point exitCode is set to the process' exit code

    // Handles must be closed when they are no longer needed
    CloseHandle(pi->hThread);
    CloseHandle(pi->hProcess);

    CloseHandle(*out);

    return exitCode;
}

#endif

int SystemInterface::compile(TranslationUnit tu, ProjectInfo *p_Proj_Info)
{
    char buffer[1024];

    const unsigned int pref_length = 21;
    char *path;
    std::filesystem::path current_include;
    int include_length = 0;

    std::vector<std::string> headers;
#if defined(_WIN32)
    HANDLE out;
    PROCESS_INFORMATION pi;
    FILE *pipe = win_popen(((p_Proj_Info->arch == X64 ? compiler64 : compiler32) + "\\cl.exe").c_str(), (char *)(" " + p_Proj_Info->comp_Flags + system_Include_Paths + "/c /showIncludes /EHsc /MD /Fo\"" + tu.o_File + "\" \"" + tu.cpp_File + "\"").c_str(), &out, &pi);

    if (!pipe)
    {
        return ERROR_CODE;
    }

    for (int i = 0; i < 4; i++)
        fgets(buffer, 1024, pipe);

    while (fgets(buffer, 1024, pipe))
    {
        if (memcmp(buffer, "Note: including file:", pref_length) == 0)
        {
            path = buffer + 21;

            while ((++path)[0] == ' ')
                ;
            include_length = strlen(path);
            while (path[include_length - 1] == '\n' || path[include_length - 1] == '\r')
            {
                path[include_length - 1] = 0;
                include_length = strlen(path);
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
        else
            printf("%s", buffer);
    }

    p_Proj_Info->header_Dependencies[tu.cpp_File] = headers;

    return win_pclose(&out, &pi) == 0 ? UPDATED_CODE : ERROR_CODE;
#elif defined(__linux__)
    FILE *pipe = popen(("g++ -march=x86-64 " + std::string(p_Proj_Info->arch == X86 ? "-m32 " : "-m64 ") + p_Proj_Info->comp_Flags + "-c -H -o \"" + tu.o_File + "\" \"" + tu.cpp_File + "\" 2>&1").c_str(), "r");

    if (!pipe)
    {
        return ERROR_CODE;
    }

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

    p_Proj_Info->header_Dependencies[tu.cpp_File] = headers;

    return pclose(pipe) == 0 ? UPDATED_CODE : ERROR_CODE;
#endif
}

int SystemInterface::link_App(ProjectInfo *p_Proj_Info)
{
    std::string o_Files = "";
    for (const auto &tu : p_Proj_Info->files)
        o_Files += "\"" + tu.o_File + "\" ";

    char buffer[1024];
#if defined(_WIN32)
    HANDLE out;
    PROCESS_INFORMATION pi;
    // TODO fix this shit
    FILE *pipe = win_popen(((p_Proj_Info->arch == X64 ? compiler64 : compiler32) + "\\link.exe").c_str(), (char *)(" /out:\"" + p_Proj_Info->output_Path + "\" " + o_Files + p_Proj_Info->libs + p_Proj_Info->link_Flags + (p_Proj_Info->arch == X64 ? system_Lib64_Paths : system_Lib32_Paths)).c_str(), &out, &pi);
#elif defined(__linux__)
    FILE *pipe = popen(("g++ -march=x86-64 " + std::string(p_Proj_Info->arch == X86 ? "-m32 " : "-m64 ") + "-o \"" + p_Proj_Info->output_Path + "\" " + o_Files + p_Proj_Info->link_Flags + +" " + p_Proj_Info->libs + " 2>&1").c_str(), "r");

    printf("running g++...\n");
#endif
    if (!pipe)
    {
        return ERROR_CODE;
    }

#if defined(_WIN32)
    for (int i = 0; i < 3; i++)
        fgets(buffer, 1024, pipe);
#endif

    while (fgets(buffer, 1024, pipe))
        printf("%s", buffer);
#if defined(_WIN32)
    return win_pclose(&out, &pi) == 0 ? UPDATED_CODE : ERROR_CODE;
#elif defined(__linux__)
    return pclose(pipe) == 0 ? UPDATED_CODE : ERROR_CODE;
#endif
}

int SystemInterface::link_Lib(ProjectInfo *p_Proj_Info)
{
    char buffer[1024];
    std::string o_Files = "";
    for (const auto &tu : p_Proj_Info->files)
        o_Files += "\"" + tu.o_File + "\" ";
#if defined(_WIN32)
    HANDLE out;
    PROCESS_INFORMATION pi;
    printf("\"%s\"\n", p_Proj_Info->output_Path.c_str());
    FILE *pipe = win_popen(((p_Proj_Info->arch == X64 ? compiler64 : compiler32) + "\\lib.exe").c_str(), (char *)(" /out:\"" + p_Proj_Info->output_Path + "\" " + o_Files).c_str(), &out, &pi);
#elif defined(__linux__)
    FILE *pipe = popen(("ar rcs \"" + p_Proj_Info->output_Path + "\" " + o_Files + " 2>&1").c_str(), "r");

    printf("running g++...\n");
#endif
    if (!pipe)
    {
        return ERROR_CODE;
    }

#if defined(_WIN32)
    for (int i = 0; i < 3; i++)
        fgets(buffer, 1024, pipe);
#endif

    while (fgets(buffer, 1024, pipe))
        printf("%s", buffer);
#if defined(_WIN32)
    return win_pclose(&out, &pi) == 0 ? UPDATED_CODE : ERROR_CODE;
#elif defined(__linux__)
    return pclose(pipe) == 0 ? UPDATED_CODE : ERROR_CODE;
#endif
}

int SystemInterface::execute_Program(const char *prog, const char *args)
{
    char buffer[1024];
#if defined(_WIN32)
    HANDLE out;
    PROCESS_INFORMATION pi;
    FILE *pipe = win_popen(prog, (char *)args, &out, &pi);
#elif defined(__linux__)
    FILE *pipe = popen(("\'" + std::string(prog) + "\' " + std::string(args == NULL ? "" : args)).c_str(), "r");
#endif
    if (!pipe)
    {
        printf(RED "error : process wasn't able to run\n" C_RESET);
        exit(-1);
    }

    while (fgets(buffer, 1024, pipe))
        printf("%s", buffer);
#if defined(_WIN32)
    return win_pclose(&out, &pi);
#elif defined(__linux__)
    return pclose(pipe);
#endif
}