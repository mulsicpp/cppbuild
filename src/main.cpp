#include "CppBuilder.h"

#include <stdio.h>

int main(int argc, char *argv[])
{
    CppBuilder builder(argc, argv);

    for(int i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }

    printf("output path: %s\n", builder.proj_Info.output_Path.c_str());
    printf("force: %i\n", builder.force);

    builder.build();

    return 0;
}