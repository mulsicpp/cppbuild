#if defined(_WIN32)
#include "windows.h"
#endif

#if defined(__linux__)
#include <unistd.h>
#include <linux/limits.h>
#endif

#include "native_commands.h"
#include "cppbuild.h"

#include <filesystem>

/*
#if defined(_WIN32)

static int winCommand(const char *app, char *cmd, int discarded_lines)
{
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(sa);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = true;

  HANDLE h = CreateFileA(".out.txt",
                         FILE_APPEND_DATA,
                         FILE_SHARE_WRITE | FILE_SHARE_READ,
                         &sa,
                         OPEN_ALWAYS,
                         FILE_ATTRIBUTE_HIDDEN,
                         NULL);

  PROCESS_INFORMATION pi;
  STARTUPINFOA si;
  BOOL ret = false;
  DWORD flags = CREATE_NO_WINDOW;

  memset(&pi, 0, sizeof(PROCESS_INFORMATION));
  memset(&si, 0, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  si.dwFlags |= STARTF_USESTDHANDLES;
  si.hStdInput = NULL;
  si.hStdError = h;
  si.hStdOutput = h;
  int success = CreateProcessA(app, cmd, nullptr, nullptr, true, flags, nullptr, nullptr, &si, &pi);

  if (success)
  {
    // Wait for the process to exit
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Process has exited - check its exit code
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    // At this point exitCode is set to the process' exit code

    // Handles must be closed when they are no longer needed
    CloseHandle(h);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    FILE *p_file = fopen(".out.txt", "r");
    char buffer[512];
    if (p_file != nullptr)
    {
      for (int i = 0; i < discarded_lines; i++)
        fgets(buffer, 512, p_file); 
      memset(buffer, 0, 512);
      while (fgets(buffer, 512, p_file))
        printf("%s", buffer); 
      fclose(p_file);
      DeleteFileA(".out.txt");
    }
    return exitCode == 0 ? _UPDATED : _ERROR;
  }
  else
  {
    return _ERROR;
  }
}
#endif

#if defined(__linux__)
static int linuxCommand(std::string command)
{
  char buffer[512];
  FILE *pipe = popen(command.c_str(), "r");

  if (!pipe)
    return _ERROR;

  while (!feof(pipe))
  {
    if (fgets(buffer, 512, pipe) != nullptr)
      printf("%s", buffer);
  }

  int rc = pclose(pipe);
  return rc == 0 ? _UPDATED : _ERROR;
}
#endif

int executeNativePreprocCommand(std::string src, std::string preproc, std::string obj, bool *needs_update)
{
#if defined(_WIN32)
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(sa);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = true;

  HANDLE h = CreateFileA(".out.txt",
                         FILE_APPEND_DATA,
                         FILE_SHARE_WRITE | FILE_SHARE_READ,
                         &sa,
                         OPEN_ALWAYS,
                         FILE_ATTRIBUTE_HIDDEN,
                         NULL);

  PROCESS_INFORMATION pi;
  STARTUPINFOA si;
  BOOL ret = false;
  DWORD flags = CREATE_NO_WINDOW;

  memset(&pi, 0, sizeof(PROCESS_INFORMATION));
  memset(&si, 0, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  si.dwFlags |= STARTF_USESTDHANDLES;
  si.hStdInput = NULL;
  si.hStdError = h;
  si.hStdOutput = h;
  int success = CreateProcessA((_path_to_exe + "\\msvc\\msvc\\" + (_arch == X86 ? "x86" : "x64") + "\\cl.exe").c_str(), (char *)((" " + _comp_flags + "/P /showIncludes /MD /Fi\"" + preproc + "\" /EHsc \"" + src + "\"").c_str()), nullptr, nullptr, true, flags, nullptr, nullptr, &si, &pi);

  if (success)
  {
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(h);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    FILE *p_file = fopen(".out.txt", "r");
    char buffer[1024];
    if (p_file != nullptr)
    {
      for (int i = 0; i < 4; i++)
        fgets(buffer, 1024, p_file); 
      memset(buffer, 0, 1024);
      const unsigned int pref_length = 22;
      char *path;
      const char *exe_path = _path_to_exe.c_str();
      const unsigned int exe_path_length = strlen(exe_path);
      int include_length = 0;
      std::filesystem::path current_include;
      while (fgets(buffer, 1024, p_file))
      {
        //check include files
        if (!*needs_update && memcmp("Note: including file: ", buffer, pref_length) == 0)
        {
          path = buffer + pref_length;
          while (path[0] == ' ')
            path++;
          include_length = strlen(path);
          path[include_length - 1] = 0;
          try
          {
            current_include = std::filesystem::canonical(std::string(path));
          }
          catch (std::filesystem::filesystem_error e)
          {
            _ERROR_MSG("path of included file \'%s\' is not valid", path);
          }
          if (memcmp(exe_path, current_include.string().c_str(), exe_path_length) == 0 && current_include.c_str()[exe_path_length] == '\\')
          {
            continue;
          }
          if (!std::filesystem::exists(obj) || std::filesystem::last_write_time(obj) < std::filesystem::last_write_time(current_include))
          {
            *needs_update = true;
          }
        }
        else
        {
          if (memcmp("Note: including file: ", buffer, pref_length))
            printf("%s", buffer);
        }
      }
      fclose(p_file);
      DeleteFileA(".out.txt");
    }
    return exitCode == 0 ? _UPDATED : _ERROR;
  }
  else
  {
    return _ERROR;
  }
#endif

#if defined(__linux__)
  char buffer[1024];
  FILE *pipe = popen(("g++ -march=x86-64 " + std::string(_arch == X86 ? "-m32 " : "-m64 ") + _comp_flags + "-E -H -o \"" + preproc + "\" \"" + src + "\" 2>&1").c_str(), "r");

  if (!pipe)
    return _ERROR;

  const unsigned int pref_length = 22;
  char *path;
  std::filesystem::path current_include;
  int include_length = 0;
  while (fgets(buffer, 1024, pipe))
  {
    //check include files
    if (!*needs_update && buffer[0] == '.')
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
          _ERROR_MSG("path of included file \'%s\' is not valid", path);
        }
        if (memcmp("/usr/include/", current_include.string().c_str(), 13) == 0)
        {
          continue;
        }
        if (std::filesystem::last_write_time(obj) < std::filesystem::last_write_time(current_include))
        {
          *needs_update = true;
        }
      }
    }
    else if (strcmp(buffer, "Multiple include guards may be useful for:\n") == 0)
    {
      break;
    }
    else
    {
      if (buffer[0] == '.')
      {
        path = buffer + 1;
        while (path[0] == '.')
          path++;
        if (path[0] != ' ')
          printf("%s", buffer);
      }
      else
        printf("%s", buffer);
    }
  }

  int rc = pclose(pipe);
  return rc == 0 ? _UPDATED : _ERROR;
#endif
}

int executeNativeCompileCommand(std::string src, std::string bin)
{
#if defined(_WIN32)
  return winCommand((std::string(_path_to_exe) + "\\msvc\\msvc\\" + (_arch == X86 ? "x86" : "x64") + "\\cl.exe").c_str(), (char *)((" " + _comp_flags + "/c /MD /Fo\"" + bin + "\" /EHsc /Tp \"" + src + "\"").c_str()), 4);
#endif

#if defined(__linux__)
  return linuxCommand("g++ -march=x86-64 " + std::string(_arch == X86 ? "-m32 " : "-m64 ") + _comp_flags + "-c -Wl,--unresolved-symbols=ignore-in-object-files -o \"" + bin + "\" \"" + src + "\"");
#endif
}

int executeNativeLibCommand(std::string lib, std::string args)
{
#if defined(_WIN32)
  return winCommand((std::string(_path_to_exe) + "\\msvc\\msvc\\" + (_arch == X86 ? "x86" : "x64") + "\\lib.exe").c_str(), (char *)(" /out:\"" + lib + "\" " + args).c_str(), 3);
#endif

#if defined(__linux__)
  return linuxCommand("ar rcs \"" + lib + "\" " + args);
#endif
}

int executeNativeLinkAppCommand(std::string exe, std::string args)
{
#if defined(_WIN32)
  return winCommand((std::string(_path_to_exe) + "\\msvc\\msvc\\" + (_arch == X86 ? "x86" : "x64") + "\\link.exe").c_str(), (char *)(" /out:\"" + exe + "\" " + args + " " + _link_flags).c_str(), 3);
#endif

#if defined(__linux__)
  return linuxCommand("g++ -march=x86-64 " + std::string(_arch == X86 ? "-m32 " : "-m64 ") + "-o \"" + exe + "\" " + args + " " + _link_flags);
#endif
}

int executeNativeLinkDLLCommand(std::string dll, std::string args)
{
#if defined(_WIN32)
  return winCommand((std::string(_path_to_exe) + "\\msvc\\msvc\\" + (_arch == X86 ? "x86" : "x64") + "\\link.exe").c_str(), (char *)(" /DLL /out:\"" + dll + "\" " + args + " " + _link_flags).c_str(), 3);
#endif

#if defined(__linux__)
  return linuxCommand("g++ -march=x86-64 " + std::string(_arch == X86 ? "-m32 " : "-m64 ") + "-shared  -o \"" + dll + "\" " + args + " " + _link_flags);
#endif
}

int executeProgram(const char *exe, char *args)
{
#if defined(_WIN32)

  PROCESS_INFORMATION pi;
  STARTUPINFOA si;

  memset(&pi, 0, sizeof(PROCESS_INFORMATION));
  memset(&si, 0, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  int success = CreateProcessA(exe, args, nullptr, nullptr, true, 0, nullptr, nullptr, &si, &pi);

  if (success)
  {
    // Wait for the process to exit
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Process has exited - check its exit code
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    // At this point exitCode is set to the process' exit code

    // Handles must be closed when they are no longer needed
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return exitCode;
  }
  else
  {
    printf(RED "error : process wasn't able to run\n" C_RESET);
    exit(-1);
  }
#endif

#if defined(__linux__)
  char buffer[512];
  FILE *pipe = popen(("\'" + std::string(exe) + "\' " + std::string(args == NULL ? "" : args)).c_str(), "r");

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

*/