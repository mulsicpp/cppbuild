#include "CppBuilder.h"

#include <stdio.h>

int main(int argc, char *argv[])
{
    printf("hello\n");

    CppBuilder builder(argc, argv);

    printf("path to exe: %s\n", builder.path_To_Exe.c_str());
    printf("path to project: %s\n", builder.path_To_Project.c_str());

    printf("arch: %s\n", builder.arch == X64 ? "x64" : "x86");
    printf("run: %i\n", builder.run);
    printf("force: %i\n", builder.force);

    return 0;
}