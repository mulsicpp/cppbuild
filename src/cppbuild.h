#pragma once

#include "stdio.h"
#include <string>

#if defined(_WIN32)
#define OS_NAME "win32"
#define OS_APP_FORMAT "$1.exe"
#define OS_LIB_FORMAT "$1.lib"
#define OS_DLL_FORMAT "$1.dll"
#define OS_PLATFORM "win32"
#define OS_INCLUDE_PATH(x) "/I" + x + " "
#define OS_LIBRARY_PATH(x) "/LIBPATH:" + x + " "
#define OS_LINK_LIBRARY(x) "\"" + x + ".lib\" "
#define OS_DEFINE(x) "/D" + x + " "
#define OS_MACRO(x, y) "/D" + x + "=" + y + " "
#define OS_STD(x) "/std:" + x + " "
#define OS_DIR_SEPARATOR "\\"

#elif defined(__linux__)
#define OS_NAME "linux"
#define OS_APP_FORMAT "$1"
#define OS_LIB_FORMAT "lib$1.a"
#define OS_DLL_FORMAT "lib$1.so"
#define OS_PLATFORM "linux"
#define OS_INCLUDE_PATH(x) "-I" + x + " "
#define OS_LIBRARY_PATH(x) "-L" + x + " "
#define OS_LINK_LIBRARY(x) "-l\"" + x + "\" "
#define OS_DEFINE(x) "-D" + x + " "
#define OS_MACRO(x, y) "-D\"" + x + "=" + y + "\" "
#define OS_STD(x) "-std=" + x + " "
#define OS_DIR_SEPARATOR "/"
#endif

//colors
#define F_RESET "\033[0m"

#define F_BLACK "\033[0;30m"
#define F_RED "\033[0;31m"
#define F_GREEN "\033[0;32m"
#define F_YELLOW "\033[0;33m"
#define F_BLUE "\033[0;34m"
#define F_PURPLE "\033[0;35m"
#define F_CYAN "\033[0;36m"
#define F_WHITE "\033[0;37m"
#define F_BOLD "\033[1m"

template <class... T>
void error(const char *str, T... args)
{
  printf(F_RED);
  printf(str, args...);
  printf("\n" F_RESET);
  exit(1);
}

template<class... T>
void msg(const char *format, const char *str, T... args)
{
  printf(format);
  printf(str, args...);
  printf("\n" F_RESET);
}

#define NEW_LINE printf("\n");

#define UPDATED_CODE 0x1
#define ERROR_CODE 0x2

#define MAX_ARG_COUNT 100
#define MAX_LINE_LENGTH 2048

bool path_Is_Contained(const std::string &base, const std::string &sub);

enum Architecture
{
  X86,
  X64
};

enum Config
{
  RELEASE,
  DEBUG
};

enum OutputType {
  NONE, APP, LIB, DLL
};