#include "CppBuilder.h"

#include <stdio.h>
#include <filesystem>

int main(int argc, char *argv[])
{
    CppBuilder builder(argc, argv);

    builder.print_Project_Info();

    builder.build();

    return 0;
}