#include "CppBuilder.h"

#include <stdio.h>

int main(int argc, char *argv[])
{
    CppBuilder builder(argc, argv);

    printf("output path: %s\n", builder.proj_Info.output_Path.c_str());
    printf("output type: %i\n", builder.proj_Info.output_Type);

    builder.build();

    return 0;
}