#include "cppbuild.h"
#include <filesystem>

bool path_Is_Contained(const std::string &base, const std::string &sub) {
    std::string relative = std::filesystem::relative(sub, base).string();
    // Size check for a "." result.
    // If the path starts with "..", it's not a subdirectory.
    return relative.size() == 1 || relative[0] != '.' && relative[1] != '.';
}