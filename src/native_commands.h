#pragma once

#include <string>

int executeNativePreprocCommand(std::string src, std::string preproc, std::string obj, bool *needs_update);

int executeNativeCompileCommand(std::string src, std::string bin);

int executeNativeLibCommand(std::string lib, std::string args);

int executeNativeLinkAppCommand(std::string exe, std::string args);

int executeNativeLinkDLLCommand(std::string dll, std::string args);

int executeProgram(const char* exe, char* args);